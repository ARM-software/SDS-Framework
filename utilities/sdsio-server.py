# Copyright (c) 2023-2026 Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the License); you may
# not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an AS IS BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import argparse
import sys
import os
import os.path as path
import serial
import usb1
import ipaddress
import ifaddr
import socket
import threading
import time
import logging
import asyncio
import ctypes
import signal
import yaml
from typing import Optional, NamedTuple

if os.name == "nt":
    from ctypes import wintypes
    import msvcrt
else:
    import select
    import termios
    import tty

SDSIO_SERVER_VERSION = "0.9.26"

class StreamInfo(NamedTuple):
    name: str = None
    mode: int = None
    file_paths: list[str] = None
    remaining_file_sizes: list[int] = None
    file_idx:int = 0

# SDSIO protocol command IDs
CMD_OPEN        = 1
CMD_CLOSE       = 2
CMD_WRITE       = 3
CMD_READ        = 4
CMD_PING        = 5
CMD_FLAGS       = 6
CMD_INFO        = 7
CMD_SYNC        = set(range(CMD_OPEN, CMD_PING + 1))    # commands with sid/arg/sz/data layout
CMD_ALL         = set(range(CMD_OPEN, CMD_INFO + 1))    # all valid command IDs

# SDSIO monitor commands and  messages
SDSIO_MON_OPEN        = 1
SDSIO_MON_CLOSE       = 2
SDSIO_MON_FLAGS       = 6
SDSIO_MON_INFO        = 7
SDSIO_MON_SHUTDOWN    = 8

# SDS Flags bit positions
SDS_FLAG_MASK_START        = (1 << 31)
SDS_FLAG_MASK_CI_TERMINATE = (1 << 30)
SDS_FLAG_MASK_PLAYBACK_MODE= (1 << 29)
SDS_FLAG_MASK_ALIVE        = (1 << 28)
SDS_FLAG_MASK_RESET        = (1 << 27)

# Mapping of human-readable parity names to pyserial constants
PARITY_NAME_MAP = {
    'none':  serial.PARITY_NONE,
    'even':  serial.PARITY_EVEN,
    'odd':   serial.PARITY_ODD,
    'mark':  serial.PARITY_MARK,
    'space': serial.PARITY_SPACE,
}

# ---------------------------------------------------------------------------- #
#           Byte oriented in memory buffer with per-stream flow control        #
# ---------------------------------------------------------------------------- #
class ByteStreamBuffer:
    def __init__(self, max_size=None):
        if max_size is None:
            _max_size = 1024 * 1024 * 1024  # default to 1 GB
        else:
            _max_size = max_size
        self._buf = bytearray(_max_size)
        self._max = _max_size
        self._head = 0      # next read position
        self._tail = 0      # next write position
        self._count = 0     # number of bytes in buffer
        self.eof = False
        self._lock = threading.Lock()
        self._not_empty = threading.Condition(self._lock)
        self._not_full  = threading.Condition(self._lock)

    def write(self, data: bytes):
        with self._not_full:
            # wait for enough free space
            while self._count + len(data) > self._max:
                self._not_full.wait()
            # write in up to two slices
            _first = min(len(data), self._max - self._tail)
            self._buf[self._tail:self._tail+_first] = data[:_first]
            self._tail = (self._tail + _first) % self._max
            _second = len(data) - _first
            if _second:
                self._buf[self._tail:self._tail+_second] = data[_first:]
                self._tail = (self._tail + _second) % self._max
            self._count += len(data)
            # wake readers
            self._not_empty.notify_all()

    def read(self, amt: int, timeout=None) -> bytes:
        with self._not_empty:
            # wait for data or EOF
            if self._count == 0 and not self.eof:
                self._not_empty.wait(timeout)
            if self._count == 0:
                return b''
            _to_read = min(amt, self._count)
            # read in up to two slices
            _first = min(_to_read, self._max - self._head)
            _data = bytes(self._buf[self._head:self._head+_first])
            self._head = (self._head + _first) % self._max
            _second = _to_read - _first
            if _second:
                _data += bytes(self._buf[self._head:self._head+_second])
                self._head = (self._head + _second) % self._max
            self._count -= _to_read
            # wake writers
            self._not_full.notify_all()
            return _data

    def set_eof(self):
        with self._lock:
            self.eof = True
            # wake any waiting readers
            self._not_empty.notify_all()


# ---------------------------------------------------------------------------- #
#                            Logging and spinner                               #
# ---------------------------------------------------------------------------- #
class ConsoleSpinner:
    _FRAMES = '|/-\\'

    def __init__(self, stream=sys.stdout, lock=None, enabled=True):
        self._stream = stream
        self._lock = lock or threading.Lock()
        self._enabled = enabled
        self._idx = 0
        self._active = False

    @property
    def lock(self):
        return self._lock

    def tick(self, active=True):
        if not self._enabled or not active:
            return
        with self._lock:
            self._tick_unlocked()

    def _tick_unlocked(self):
        _frame = self._FRAMES[self._idx % len(self._FRAMES)]
        self._stream.write(f"\r{_frame}")
        self._stream.flush()
        self._idx += 1
        self._active = True

    def clear_unlocked(self):
        if self._enabled and self._active:
            self._stream.write("\r \r")
            self._stream.flush()
            self._active = False
            self._idx = 0


class SpinnerAwareStreamHandler(logging.StreamHandler):
    def __init__(self, stream, spinner: ConsoleSpinner):
        super().__init__(stream)
        self._spinner = spinner

    def emit(self, record):
        with self._spinner.lock:
            self._spinner.clear_unlocked()
            super().emit(record)


def setup_logger(level=logging.INFO, formatter=None, log_file=None):
    _logger = logging.getLogger("sdsio")
    for _handler in _logger.handlers[:]:
        _logger.removeHandler(_handler)
        _handler.close()
    _logger.setLevel(level)
    _logger.propagate = False

    if isinstance(formatter, str):
        _formatter = logging.Formatter(formatter)
    elif formatter is None:
        _formatter = logging.Formatter("[%(levelname)s] %(message)s")
    else:
        _formatter = formatter

    _terminal_lock = threading.Lock()
    _spinner = ConsoleSpinner(sys.stdout, _terminal_lock, enabled=(log_file is None))

    if log_file:
        if path.exists(log_file):
            _bak = log_file + ".bak"
            if path.exists(_bak):
                try:
                    os.remove(_bak)
                except Exception:
                    pass
            try:
                os.rename(log_file, _bak)
            except Exception:
                pass
        _handler = logging.FileHandler(log_file, encoding='utf-8')
    else:
        _handler = SpinnerAwareStreamHandler(sys.stdout, _spinner)

    _handler.setFormatter(_formatter)
    _logger.addHandler(_handler)
    return _logger, _spinner


# Global logger and spinner, reconfigured by main() after argument parsing.
logger, spinner = setup_logger(level=logging.INFO, formatter="%(message)s")

# ---------------------------------------------------------------------------- #
#                            Print status bar                                  #
# ---------------------------------------------------------------------------- #
class StatusBar:
    def __init__(self, manager, interval=0.3):
        self._mgr = manager
        self._interval = interval
        self._stop_event = threading.Event()
        self._thread = threading.Thread(target=self._run, daemon=True)
        self._thread.start()

    def _run(self):
        while not self._stop_event.is_set():
            if (self._mgr.time_last_rw + self._interval) > time.time():
                spinner.tick(active=bool(self._mgr.opened_streams))
            time.sleep(self._interval)

    def stop(self):
        self._stop_event.set()
        self._thread.join()

# ---------------------------------------------------------------------------- #
#                          SDS IO Monitor interface                            #
# ---------------------------------------------------------------------------- #
class sdsMonitorInterface():
    def __init__(self, port, flags: Optional['sdsFlags'] = None):
        self._port = port
        self._flags = flags
        self._lock = threading.Lock()     # guards self._socket
        self._recv_buf = bytearray()      # only accessed from handle_commands thread
        self._last_info = (0, 0, b'')     # cached (flags, idle_rate, err_data) for new clients
        if self._port:
            logger.info(f"Starting monitor server on port {self._port}.")
            self._listening_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self._listening_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            self._listening_socket.bind(('127.0.0.1', self._port))
            self._listening_socket.listen(1)
            self._listening_socket.settimeout(0.01)
        else:
            self._listening_socket = None
        self._socket = None

    def _accept_client(self):
        try:
            _soc, _ = self._listening_socket.accept()
            _soc.setblocking(False)
            with self._lock:
                self._socket = _soc
            self._recv_buf.clear()
            logger.info("Monitor client connected.")
            # Send current info immediately on connection
            self.send_info_msg(*self._last_info)
        except socket.timeout:
            pass
        except Exception:
            pass

    def handle_commands(self):
        if self._listening_socket is None:
            return False

        if self._socket is None:
            self._accept_client()

        if self._socket is None:
            return False

        # Receive available bytes into the buffer
        _soc = self._socket
        try:
            _chunk = _soc.recv(256)
            if _chunk:
                self._recv_buf.extend(_chunk)
            else:
                # Peer closed connection gracefully
                logger.info("Monitor client disconnected.")
                with self._lock:
                    if self._socket is _soc:
                        try:
                            self._socket.close()
                        except Exception:
                            pass
                        self._socket = None
                self._recv_buf.clear()
                return False
        except BlockingIOError:
            pass  # No data available right now
        except Exception:
            logger.info("Monitor client disconnected.")
            with self._lock:
                if self._socket is _soc:
                    try:
                        self._socket.close()
                    except Exception:
                        pass
                    self._socket = None
            self._recv_buf.clear()
            return False

        # Process all complete 16-byte commands in the buffer
        while len(self._recv_buf) >= 16:
            _cmd = int.from_bytes(self._recv_buf[0:4], 'little')
            if _cmd == SDSIO_MON_FLAGS:
                _set_flags  = int.from_bytes(self._recv_buf[4:8],  'little')
                _clear_flags = int.from_bytes(self._recv_buf[8:12], 'little')
                logger.info(f"Monitor command received: SDSIO_MON_FLAGS (set=0x{_set_flags:08X}, clear=0x{_clear_flags:08X}).")
                if self._flags:
                    self._flags.apply(_set_flags, _clear_flags)
            elif _cmd == SDSIO_MON_SHUTDOWN:
                logger.info("Monitor command received: SDSIO_MON_SHUTDOWN.")
                del self._recv_buf[:16]
                return True
            else:
                logger.warning(f"Unknown monitor command received: {_cmd}.")
            del self._recv_buf[:16]
        return False

    def _send(self, msg: bytearray):
        """Send msg to the monitor client under the lock."""
        with self._lock:
            if self._socket:
                try:
                    self._socket.sendall(msg)
                except Exception:
                    pass

    def send_open_msg(self, filename: str, mode: int):
        _filename_bytes = filename.encode('utf-8')
        _msg = bytearray()
        _msg.extend(SDSIO_MON_OPEN.to_bytes(4, 'little'))
        _msg.extend((0).to_bytes(4, 'little'))
        _msg.extend(mode.to_bytes(4, 'little'))
        _msg.extend((0).to_bytes(4, 'little'))
        _msg.extend(len(_filename_bytes).to_bytes(4, 'little'))
        _msg.extend(_filename_bytes)
        self._send(_msg)

    def send_close_msg(self, filename: str):
        _filename_bytes = filename.encode('utf-8')
        _msg = bytearray()
        _msg.extend(SDSIO_MON_CLOSE.to_bytes(4, 'little'))
        _msg.extend((0).to_bytes(4, 'little'))
        _msg.extend((0).to_bytes(4, 'little'))
        _msg.extend((0).to_bytes(4, 'little'))
        _msg.extend(len(_filename_bytes).to_bytes(4, 'little'))
        _msg.extend(_filename_bytes)
        self._send(_msg)

    def send_info_msg(self, flags: int, idle_rate: int, err_data: bytes):
        self._last_info = (flags, idle_rate, err_data)
        _msg = bytearray()
        _msg.extend(SDSIO_MON_INFO.to_bytes(4, 'little'))
        _msg.extend(flags.to_bytes(4, 'little'))
        _msg.extend(idle_rate.to_bytes(4, 'little'))
        _msg.extend(len(err_data).to_bytes(4, 'little'))
        _msg.extend(err_data)
        self._send(_msg)

    def close(self):
        with self._lock:
            if self._socket:
                try:
                    self._socket.close()
                except Exception:
                    pass
                self._socket = None
        if self._listening_socket:
            try:
                self._listening_socket.close()
            except Exception:
                pass
            self._listening_socket = None


# ---------------------------------------------------------------------------- #
#                               SDS IO Flags                                   #
# ---------------------------------------------------------------------------- #
class sdsFlags:
    def __init__(self, auto_playback=False):
        self._set   = 0
        self._clear = 0
        self._lock  = threading.Lock()
        self._auto_playback = auto_playback
        self._playback_mode = auto_playback
        self._target_flags = 0
        self._auto_start_pending = False
        self._auto_terminate_pending = False
        if auto_playback:
            self._set = SDS_FLAG_MASK_PLAYBACK_MODE | SDS_FLAG_MASK_START
            self._auto_start_pending = True

    def apply(self, set_mask: int, clear_mask: int):
        with self._lock:
            self._set    = (self._set | set_mask) & ~clear_mask
            self._clear  = (self._clear | clear_mask) & ~set_mask
            if set_mask & SDS_FLAG_MASK_PLAYBACK_MODE:
                self._playback_mode = True
            if not self._auto_playback and (clear_mask & SDS_FLAG_MASK_PLAYBACK_MODE):
                self._playback_mode = False

    def update_from_target(self, flags: int):
        with self._lock:
            self._target_flags = flags
            if flags & SDS_FLAG_MASK_START:
                self._auto_start_pending = False
            if flags & SDS_FLAG_MASK_PLAYBACK_MODE:
                self._playback_mode = True
            elif not self._auto_playback:
                self._playback_mode = False

    def request_auto_playback_start(self) -> bool:
        with self._lock:
            if not self._auto_playback:
                return False
            if self._auto_terminate_pending:
                return False
            if self._auto_start_pending or (self._target_flags & SDS_FLAG_MASK_START):
                return False
            self._set |= SDS_FLAG_MASK_PLAYBACK_MODE | SDS_FLAG_MASK_START
            self._clear &= ~(SDS_FLAG_MASK_PLAYBACK_MODE | SDS_FLAG_MASK_START)
            self._playback_mode = True
            self._auto_start_pending = True
            return True

    def request_auto_playback_terminate(self) -> bool:
        with self._lock:
            if not self._auto_playback:
                return False
            if self._auto_start_pending or self._auto_terminate_pending:
                return False
            if self._target_flags & SDS_FLAG_MASK_START:
                return False
            self._set |= SDS_FLAG_MASK_CI_TERMINATE
            self._clear &= ~SDS_FLAG_MASK_CI_TERMINATE
            self._auto_terminate_pending = True
            return True

    def consume_set(self) -> int:
        with self._lock:
            _val = self._set | SDS_FLAG_MASK_ALIVE
            if _val & SDS_FLAG_MASK_PLAYBACK_MODE:
                self._playback_mode = True
            self._set = 0
            return _val

    def consume_clear(self) -> int:
        with self._lock:
            _val = self._clear
            if not self._auto_playback and (_val & SDS_FLAG_MASK_PLAYBACK_MODE):
                self._playback_mode = False
            self._clear = 0
            return _val

    @property
    def playback_mode(self):
        return self._playback_mode

    @property
    def auto_playback(self):
        return self._auto_playback

    @property
    def target_flags(self):
        return self._target_flags


# ---------------------------------------------------------------------------- #
#                            SDS IO Control Input                              #
# ---------------------------------------------------------------------------- #
class sdsControlInput(threading.Thread):

    def __init__(self, flags: sdsFlags, monitor: Optional[sdsMonitorInterface] = None, shutdown_event: Optional[threading.Event] = None):
        super().__init__(daemon=True)
        self._flags          = flags
        self._monitor        = monitor
        self._shutdown_event = shutdown_event
        self._quit           = threading.Event()
        # Capture the running event loop and main task so that X/x can request
        # a clean asyncio cancellation instead of raising KeyboardInterrupt via
        # os.kill(), which bypasses Python try/except on Windows.
        try:
            self._loop      = asyncio.get_running_loop()
            self._main_task = asyncio.current_task()
        except RuntimeError:
            self._loop      = None
            self._main_task = None

        # Mapping of key characters to (set_mask, clear_mask, description)
        self._KEY_ACTIONS = {
            'R': (SDS_FLAG_MASK_START,                               SDS_FLAG_MASK_PLAYBACK_MODE, "start recording"),
            'r': (SDS_FLAG_MASK_START,                               SDS_FLAG_MASK_PLAYBACK_MODE, "start recording"),
            'P': (SDS_FLAG_MASK_START | SDS_FLAG_MASK_PLAYBACK_MODE, 0,                           "start playback"),
            'p': (SDS_FLAG_MASK_START | SDS_FLAG_MASK_PLAYBACK_MODE, 0,                           "start playback"),
            'S': (0,                                                 SDS_FLAG_MASK_START,         "stop"),
            's': (0,                                                 SDS_FLAG_MASK_START,         "stop"),
            'T': (SDS_FLAG_MASK_RESET,                               0,                           "reset target"),
            't': (SDS_FLAG_MASK_RESET,                               0,                           "reset target"),
            'A': (1 << 0, 0,       "set flag 0"),   'a': (0, 1 << 0, "clear flag 0"),
            'B': (1 << 1, 0,       "set flag 1"),   'b': (0, 1 << 1, "clear flag 1"),
            'C': (1 << 2, 0,       "set flag 2"),   'c': (0, 1 << 2, "clear flag 2"),
            'D': (1 << 3, 0,       "set flag 3"),   'd': (0, 1 << 3, "clear flag 3"),
            'E': (1 << 4, 0,       "set flag 4"),   'e': (0, 1 << 4, "clear flag 4"),
            'F': (1 << 5, 0,       "set flag 5"),   'f': (0, 1 << 5, "clear flag 5"),
            'G': (1 << 6, 0,       "set flag 6"),   'g': (0, 1 << 6, "clear flag 6"),
            'H': (1 << 7, 0,       "set flag 7"),   'h': (0, 1 << 7, "clear flag 7"),
        }

        self.start()

    def _request_shutdown(self, source: str):
        logger.info(f"sdsControl: terminate ({source}).")
        if self._shutdown_event:
            self._shutdown_event.set()
        if self._loop and self._main_task:
            # Schedule cooperative cancellation through asyncio so that the
            # server's shutdown-flags path runs on all platforms.
            self._loop.call_soon_threadsafe(self._main_task.cancel)
        else:
            # Fallback for non-asyncio usage (serial-only, tests, …)
            os.kill(os.getpid(), signal.SIGINT)

    def run(self):
        while not self._quit.is_set():

            # Poll monitor for incoming commands
            if self._monitor:
                if self._monitor.handle_commands():
                    self._request_shutdown("monitor")
                    return

            # Read from keyboard
            _ch = self._read_key(timeout=0.01)
            if _ch is None:
                continue
            if _ch in ('x', 'X'):
                self._request_shutdown(f"'{_ch}'")
                return
            _action = self._KEY_ACTIONS.get(_ch)
            if _action:
                _set_mask, _clear_mask, _desc = _action
                self._flags.apply(_set_mask, _clear_mask)
                logger.info(f"sdsControl: {_desc} ('{_ch}').")

    def stop(self):
        self._quit.set()

    if os.name == "nt":
        def _read_key(self, timeout=0.1):
            _end_time = time.time() + timeout
            while time.time() < _end_time and not self._quit.is_set():
                if msvcrt.kbhit():
                    _ch = msvcrt.getwch()

                    # Ignore special keys like arrows and function keys.
                    if _ch in ("\x00", "\xe0"):
                        if msvcrt.kbhit():
                            msvcrt.getwch()
                        return None

                    return _ch

                time.sleep(0.01)
            return None
    else:
        def _read_key(self, timeout=0.1):
            import select as _select
            import termios as _termios
            import tty as _tty

            _fd = sys.stdin.fileno()
            _old_settings = _termios.tcgetattr(_fd)
            try:
                _tty.setcbreak(_fd)
                _ready, _, _ = _select.select([sys.stdin], [], [], timeout)
                if _ready:
                    return sys.stdin.read(1)
                return None
            finally:
                _termios.tcsetattr(_fd, _termios.TCSADRAIN, _old_settings)


# ---------------------------------------------------------------------------- #
#                            SDS IO Manager                                    #
# ---------------------------------------------------------------------------- #
class sdsio_manager:
    def __init__(
        self,
        work_dir,
        auto_playback=False,
        play_list: Optional[list] = None,
        mon_port: Optional[int] = None,
        write_flush_size: int = 0,
        status_bar_factory=None,
        monitor_factory=None,
        control_input_factory=None,
    ):
        self._stream_id = 0
        self._play_step_index = 0
        self._work_dir = path.normpath(work_dir)
        self._rec_dir = self._work_dir
        self.opened_streams = {}    # sid -> StreamInfo
        self._label_list = []
        self._timestamp_boundaries = []
        # write side
        self._write_buffers = {}     # sid -> ByteStreamBuffer
        self._write_threads = {}     # sid -> Thread
        self._write_stop = {}        # sid -> Event
        # read side
        self._read_buffers = {}      # sid -> ByteStreamBuffer
        self._read_threads = {}      # sid -> Thread
        self._read_stop = {}         # sid -> Event
        # lock to protect stream_id increment and open checks
        self._manager_lock = threading.Lock()
        # timestamp of last stream read or write command
        self.time_last_rw = time.time()
        # status bar
        self._status = None
        if status_bar_factory is None:
            status_bar_factory = StatusBar
        if status_bar_factory:
            self._status = status_bar_factory(self)

        self._playback_mode = False
        self._play_list = play_list
        self._mon_port = mon_port
        self._write_flush_size = write_flush_size
        # SDS Control Flags
        self.shutdown_requested = threading.Event()
        self._flags = sdsFlags(auto_playback)
        self._monitor = None
        if self._mon_port and monitor_factory is not False:
            if monitor_factory is None:
                monitor_factory = sdsMonitorInterface
            if monitor_factory:
                self._monitor = monitor_factory(self._mon_port, self._flags)
        self._ctrl_input = None
        if control_input_factory is not False:
            if control_input_factory is None:
                control_input_factory = sdsControlInput
            if control_input_factory:
                logger.info("Starting SDS Control Flags thread. R=record, P=playback, S/s=stop, X/x=terminate, A-H=set flags 0-7, a-h=clear flags 0-7.")
                self._ctrl_input = control_input_factory(self._flags, self._monitor, self.shutdown_requested)

        self._info_flags: int = 0
        self._info_IdleRate: int = 0
        self._last_async_time = time.time()
        self._last_playback_stream_name = None

    def shutdown(self):
        self.shutdown_requested.set()
        if self._ctrl_input:
            self._ctrl_input.stop()
            self._ctrl_input.join(timeout=2.0)
            self._ctrl_input = None
        if self._monitor:
            self._monitor.close()
            self._monitor = None
        if self._status:
            self._status.stop()
            self._status = None
        self.clean()

    def _make_sds_file_path(self, name: str, label: str, mode: int) -> str:
        _suffix = ".p.sds" if mode == 1 and self._playback_mode else ".sds"
        _dir = self._rec_dir if mode == 1 else self._work_dir
        return path.join(_dir, f"{name}.{label}{_suffix}")

    def _build_stream_file_paths(self, name: str, mode: int) -> list[str]:
        return [self._make_sds_file_path(name, _label, mode) for _label in self._label_list]

    def _build_timestamp_boundaries(self, file_paths: list[str]) -> list[int]:
        _timestampes = []
        for _sds_file_path in file_paths:
            # Open playback sds file and read timestamp of first packet
            with open(_sds_file_path, "rb") as _f:
                _header = _f.read(8)
                if len(_header) == 8:
                    _ts = int.from_bytes(_header[0:4], 'little')
                    _timestampes.append(_ts)
                else:
                    # Error: cant get timestamp
                    _timestampes.append(0xFFFFFFFF)
        return _timestampes

    def _build_file_sizes(self, file_paths: list[str]) -> list[int]:
        _sizes = []
        for _sds_file_path in file_paths:
            try:
                _sz = os.path.getsize(_sds_file_path)
                _sizes.append(_sz)
            except Exception:
                _sizes.append(0)
        return _sizes

    def _file_write_worker(self, sid, name, buf: ByteStreamBuffer, stop_evt):
        _data = bytearray()
        try:
            if self._playback_mode:
                # In playback mode, wait until label_list and timestamp_boundaries are populated
                # timestamp_boundaries is generted when playback file is opened
                while not self._label_list or not self._timestamp_boundaries:
                    if stop_evt.is_set() or buf.eof:
                        return
                    time.sleep(0.1)

            _stream = self.opened_streams[sid]
            for _index, _sds_file_path in enumerate(_stream.file_paths):
                if stop_evt.is_set():
                    break

                # Check if sds file exsist. If so, rename it to *.bak
                if path.exists(_sds_file_path):
                    if path.exists(_sds_file_path + ".bak"):
                        try:
                            os.remove(_sds_file_path + ".bak")
                        except Exception:
                            logger.warning(f"Could not delete backup file '{_sds_file_path}.bak'.")
                    try:
                        os.rename(_sds_file_path, _sds_file_path + ".bak")
                    except Exception:
                        logger.warning(f"Could not create backup for existing file '{_sds_file_path}'.")

                _eof_reached = False
                with open(_sds_file_path, "wb") as _file_obj:
                    _bytes_since_flush = 0
                    if _index > 0:
                        # First file open was already notified in _open(); notify for subsequent files here
                        logger.info(f"Record:   {_stream.name} ({_sds_file_path}).")
                        if self._monitor:
                            self._monitor.send_open_msg(_sds_file_path, 1)
                        self.opened_streams[sid] = self.opened_streams[sid]._replace(file_idx=_index)
                    while True:
                        _data_sz = len(_data)

                        if _data_sz < 8:
                            # Accumulate header bytes
                            _chunk = buf.read(8 - _data_sz, timeout=0.1)
                            if _chunk:
                                _data += _chunk
                            elif buf.eof:
                                # Incomplete header at EOF — discard fragment and stop
                                _data = bytearray()
                                _eof_reached = True
                                break
                            elif stop_evt.is_set():
                                # Read timed out and stream was closed — no more data expected
                                _data = bytearray()
                                _eof_reached = True
                                break
                            # else: timeout, try again

                        else:
                            # Full 8-byte header available
                            _timestamp = int.from_bytes(_data[0:4], 'little')
                            _data_block_size = int.from_bytes(_data[4:8], 'little')

                            # Check if this record marks the boundary of the next label
                            if self._timestamp_boundaries and _index + 1 < len(self._timestamp_boundaries):
                                if _timestamp == self._timestamp_boundaries[_index + 1]:
                                    break  # keep data — it will be written to the next label file

                            _needed = 8 + _data_block_size
                            if _data_sz < _needed:
                                # Accumulate payload bytes
                                _chunk = buf.read(_needed - _data_sz, timeout=0.1)
                                if _chunk:
                                    _data += _chunk
                                elif buf.eof:
                                    # Incomplete record at EOF — discard and stop
                                    _data = bytearray()
                                    _eof_reached = True
                                    break
                                elif stop_evt.is_set():
                                    # Read timed out and stream was closed — no more data expected
                                    _data = bytearray()
                                    _eof_reached = True
                                    break
                                # else: timeout, try again
                            else:
                                # Complete record: write and reset for next record
                                _file_obj.write(_data)
                                _bytes_since_flush += len(_data)
                                if self._write_flush_size and _bytes_since_flush >= self._write_flush_size:
                                    _file_obj.flush()
                                    os.fsync(_file_obj.fileno())
                                    _bytes_since_flush = 0
                                _data = bytearray()
                    if self._write_flush_size and _bytes_since_flush:
                        _file_obj.flush()
                        os.fsync(_file_obj.fileno())
                # Last file close is handled in close(); send close only for non-last files
                if not _eof_reached and _index < len(_stream.file_paths) - 1:
                    logger.info(f"Closed:   {name} ({_sds_file_path}).")
                    if self._monitor:
                        self._monitor.send_close_msg(_sds_file_path)
                if _eof_reached:
                    break
        except Exception:
            logger.exception(f"Writer {sid} error.")

    def _file_read_worker(self, sid, name, buf: ByteStreamBuffer, stop_evt):
        _chunk_size = 128 * 1024
        try:
            _stream = self.opened_streams[sid]
            for _idx, _sds_file_path in enumerate(_stream.file_paths):
                if stop_evt.is_set():
                    break
                with open(_sds_file_path, "rb") as _file_obj:
                    while not stop_evt.is_set():
                        _data = _file_obj.read(_chunk_size)
                        if _data:
                            buf.write(_data)
                        else:
                            break  # EOF on this file, move to next label
                # Close notifications are tracked via remaining_file_sizes in read(); last close is in close()
        except Exception:
            logger.exception(f"Reader {sid} error.")
        finally:
            buf.set_eof()

    def _create_play_label_list(self, name) -> list[str]:
        _labels = []
        if self._play_list and self._play_step_index < len(self._play_list):
            _step = self._play_list[self._play_step_index]
            _labels = list(_step.get('labels', []))
        else:
            # No playlist: one file per open, indexed by play_step_index
            _candidate = path.join(self._work_dir, f"{name}.{self._play_step_index}.sds")
            if path.exists(_candidate):
                _labels.append(str(self._play_step_index))
        return _labels

    def _has_next_auto_playback_step(self) -> bool:
        if not self._flags.auto_playback or self.opened_streams:
            return False
        if self._play_list:
            return self._play_step_index < len(self._play_list)
        if self._last_playback_stream_name:
            return bool(self._create_play_label_list(self._last_playback_stream_name))
        return False

    def _request_auto_playback_if_needed(self, target_flags: Optional[int] = None):
        _target_flags = self._flags.target_flags if target_flags is None else target_flags
        if _target_flags & SDS_FLAG_MASK_START:
            return
        if self.opened_streams:
            return
        if self._has_next_auto_playback_step():
            if self._flags.request_auto_playback_start():
                logger.info(f"sdsControl: auto playback step {self._play_step_index}.")
        elif self._flags.auto_playback and self._last_playback_stream_name:
            if self._flags.request_auto_playback_terminate():
                logger.info("sdsControl: auto playback terminate.")

    def _open(self, mode, name):
        _cmd = CMD_OPEN
        # prepare error response
        _resp_err = bytearray()
        _resp_err.extend(_cmd.to_bytes(4,'little'))
        _resp_err.extend((0).to_bytes(4,'little'))
        _resp_err.extend(mode.to_bytes(4,'little'))
        _resp_err.extend((0).to_bytes(4,'little'))

        # validate name
        if len(name) == 0:
            logger.info(f"Invalid stream name: {name}.")
            return _resp_err
        _invalid_chars = [chr(_i) for _i in range(0x00, 0x10)] + [chr(0x7F), '"', '*', '/', ':', '<', '>', '?', '\\', '|']
        if any(_ch in name for _ch in _invalid_chars):
            logger.info(f"Invalid stream name: {name}.")
            return _resp_err

        # ensure not already open
        if any(_stream.name == name for _stream in self.opened_streams.values()):
            logger.info(f"Stream '{name}' is already opened, cannot open again.")
            return _resp_err

        # update playback mode from flags only when new session is started
        if not self.opened_streams:
            self._playback_mode = self._flags.playback_mode

        # check if new rec/play session
        if self._playback_mode:
            if not self._label_list:
                # Get flags, Set working dir
                if self._play_list:
                    if self._play_step_index < len(self._play_list):
                        _step = self._play_list[self._play_step_index]
                        _set_flags = _step.get('setflags', 0)
                        _clear_flags = _step.get('clearflags', 0)
                        _recdir = _step.get('recdir', None)
                        if _recdir:
                            self._rec_dir = path.normpath(_recdir if path.isabs(_recdir) else path.join(self._work_dir, _recdir))
                        else:
                            self._rec_dir = self._work_dir
                    else:
                        logger.error(f"Open Failed. End of playlist. No more steps available for playback stream '{name}'.")
                        return _resp_err
                else:
                    _set_flags = 0
                    _clear_flags = 0

                if _set_flags or _clear_flags:
                    logger.debug(f"Applying flags for playback stream '{name}': set=0x{_set_flags:08X}, clear=0x{_clear_flags:08X}.")
                    self._flags.apply(_set_flags, _clear_flags)

                # Create label list
                _play_label_list = self._create_play_label_list(name)
                if not _play_label_list:
                    if not self._play_list and self._play_step_index > 0:
                        logger.error(f"Open Failed. No more files available for playback stream '{name}'.")
                    else:
                        logger.error(f"Open Failed. No files found for playback stream '{name}'.")
                    return _resp_err
                self._label_list = _play_label_list

        else:
            if mode == 0:
                logger.info(f"Cannot open stream '{name}' for playback. Playback mode is not enabled.")
                return _resp_err

        # mode 1 = write, 0 = read
        if mode == 1:
            if not self._label_list:
                if not self._playback_mode:
                    # find first available numeric label for new recording
                    _idx = 0
                    _sds_file_path = self._make_sds_file_path(name, str(_idx), mode)
                    while path.exists(_sds_file_path):
                        _idx += 1
                        _sds_file_path = self._make_sds_file_path(name, str(_idx), mode)
                    self._label_list.append(str(_idx))

            _file_paths = self._build_stream_file_paths(name, mode)
            # validate if recdir exsist
            if _file_paths and not path.exists(path.dirname(_file_paths[0])):
                logger.error(f"Recording directory does not exist for stream '{name}': {path.dirname(_file_paths[0])}.")
                return _resp_err

            # allocate new sid
            with self._manager_lock:
                self._stream_id += 1
                _sid = self._stream_id
            self.opened_streams[_sid] = StreamInfo(name=name, mode=mode, file_paths=_file_paths)
            _buf = ByteStreamBuffer()
            _stop_evt = threading.Event()
            _thr = threading.Thread(
                target=self._file_write_worker,
                args=(_sid, name, _buf, _stop_evt),
                daemon=True
            )
            _thr.start()
            self._write_buffers[_sid] = _buf
            self._write_threads[_sid] = _thr
            self._write_stop[_sid]   = _stop_evt
            # Notify monitor for the first file; subsequent files are notified in the write worker
            if _file_paths:
                logger.info(f"Record:   {name} ({_file_paths[0]}).")
                if self._monitor:
                    self._monitor.send_open_msg(_file_paths[0], 1)

        else:
            _file_paths = self._build_stream_file_paths(name, mode)
            # Validate that files for all labels exist
            for _sds_file_path in _file_paths:
                if not path.exists(_sds_file_path):
                    logger.error(f"Missing file for playback stream '{name}': {_sds_file_path}.")
                    return _resp_err

            if not self._timestamp_boundaries:
                self._timestamp_boundaries = self._build_timestamp_boundaries(_file_paths)

            # allocate new sid
            with self._manager_lock:
                self._stream_id += 1
                _sid = self._stream_id

            self.opened_streams[_sid] = StreamInfo(name=name, mode=mode,
                                                  file_paths=_file_paths,
                                                  remaining_file_sizes=self._build_file_sizes(_file_paths),
                                                  file_idx=0)

            _buf = ByteStreamBuffer()
            _stop_evt = threading.Event()
            _thr = threading.Thread(
                target=self._file_read_worker,
                args=(_sid, name, _buf, _stop_evt),
                daemon=True
            )
            _thr.start()
            self._read_buffers[_sid] = _buf
            self._read_threads[_sid] = _thr
            self._read_stop[_sid]  = _stop_evt
            # Notify monitor for the first file; subsequent files are notified in the read worker
            if _file_paths:
                logger.info(f"Playback: {name} ({_file_paths[0]}).")
                if self._monitor:
                    self._monitor.send_open_msg(_file_paths[0], 0)
            self._last_playback_stream_name = name

        # build success response
        _resp = bytearray()
        _resp.extend(_cmd.to_bytes(4,'little'))
        _resp.extend(_sid.to_bytes(4,'little'))
        _resp.extend(mode.to_bytes(4,'little'))
        _resp.extend((0).to_bytes(4,'little'))

        return _resp

    def _close(self, sid):
        _resp = bytearray()
        _stream = self.opened_streams[sid]
        _name = _stream.name

        # clean up writer side
        if sid in self._write_buffers:
            _buf = self._write_buffers.pop(sid)
            _buf.set_eof()
            self._write_threads[sid].join()
            self._write_stop[sid].set()
            # Send close notification for the last written file (all previous were sent in the write worker)
            _last_stream = self.opened_streams[sid]
            if _last_stream.file_paths:
                _last_idx = _last_stream.file_idx
                if _last_idx < len(_last_stream.file_paths):
                    _sds_file_path = _last_stream.file_paths[_last_idx]
                    logger.info(f"Closed:   {_name} ({_sds_file_path}).")
                    if self._monitor:
                        self._monitor.send_close_msg(_sds_file_path)
            self._write_threads.pop(sid)
            self._write_stop.pop(sid)
        # clean up reader side
        if sid in self._read_buffers:
            self._read_stop[sid].set()
            self._read_threads[sid].join()
            # Send close notification for the last read file (previous non-last closes sent in read())
            _last_stream = self.opened_streams[sid]
            if _last_stream.file_paths:
                _last_idx = _last_stream.file_idx
                if _last_idx < len(_last_stream.file_paths):
                    _sds_file_path = _last_stream.file_paths[_last_idx]
                    logger.info(f"Closed:   {_name} ({_sds_file_path}).")
                    if self._monitor:
                        self._monitor.send_close_msg(_sds_file_path)
            self._read_buffers.pop(sid)
            self._read_threads.pop(sid)
            self._read_stop.pop(sid)
        # unregister stream
        self.opened_streams.pop(sid, None)

        if not self.opened_streams:
            if self._playback_mode:
                self._play_step_index += 1
            self._label_list.clear()
            self._timestamp_boundaries.clear()
            self._request_auto_playback_if_needed()

        return _resp

    def _write(self, sid, data):
        _resp = bytearray()
        _buf = self._write_buffers.get(sid)
        if not _buf:
            logger.info(f"Not opened for write: {sid}.")
            return _resp
        _buf.write(data)

        self.time_last_rw = time.time()
        return _resp

    def _read(self, sid, size):
        _resp = bytearray()
        _cmd = CMD_READ
        _eof = 0
        _data = bytearray()
        _entry = self.opened_streams.get(sid)
        # invalid read
        if not _entry or _entry.mode != 0:
            _resp.extend(_cmd.to_bytes(4,'little'))
            _resp.extend(sid.to_bytes(4,'little'))
            _resp.extend((0).to_bytes(4,'little'))
            _resp.extend((0).to_bytes(4,'little'))
            return _resp

        _buf = self._read_buffers.get(sid)
        # read until requested size or EOF
        while len(_data) < size:
            _chunk = _buf.read(size - len(_data), timeout=0.05)
            if not _chunk:
                break
            _data.extend(_chunk)
            # Track how much of each file has been forwarded; trigger close/advance on file boundary
            _stream = self.opened_streams.get(sid)
            if _stream and _stream.remaining_file_sizes is not None:
                _chunk_remaining = len(_chunk)
                while _chunk_remaining > 0:
                    _stream = self.opened_streams.get(sid)
                    if not _stream or _stream.remaining_file_sizes is None:
                        break
                    _f_idx = _stream.file_idx
                    if _f_idx >= len(_stream.remaining_file_sizes):
                        break

                    _file_remaining = _stream.remaining_file_sizes[_f_idx]
                    if _file_remaining > 0:
                        _consumed = min(_chunk_remaining, _file_remaining)
                        _stream.remaining_file_sizes[_f_idx] -= _consumed
                        _chunk_remaining -= _consumed

                    if _stream.remaining_file_sizes[_f_idx] <= 0:
                        _stream.remaining_file_sizes[_f_idx] = 0
                        if _stream.file_paths and _f_idx < len(_stream.file_paths) - 1:
                            # Non-last file fully drained: report close and advance to next file
                            _sds_file_path = _stream.file_paths[_f_idx]
                            logger.info(f"Closed:   {_stream.name} ({_sds_file_path}).")
                            if self._monitor:
                                self._monitor.send_close_msg(_sds_file_path)
                            self.opened_streams[sid] = self.opened_streams[sid]._replace(file_idx=_f_idx + 1)
                            _next_idx = _f_idx + 1
                            if _next_idx < len(_stream.file_paths):
                                # First file open was already notified in _open(); notify for subsequent files here
                                _sds_file_path = _stream.file_paths[_next_idx]
                                logger.info(f"Playback: {_stream.name} ({_sds_file_path}).")
                                if self._monitor:
                                    self._monitor.send_open_msg(_sds_file_path, 0)
                            continue
                        break
        if not _data and _buf.eof:
            _eof = 1
        _resp.extend(_cmd.to_bytes(4,'little'))
        _resp.extend(sid.to_bytes(4,'little'))
        _resp.extend(_eof.to_bytes(4,'little'))
        _resp.extend(len(_data).to_bytes(4,'little'))
        if _data:
            _resp.extend(_data)

        self.time_last_rw = time.time()
        return _resp

    def _pingServer(self, sid):
        _resp = bytearray()
        _cmd = CMD_PING
        _resp.extend(_cmd.to_bytes(4,'little'))
        _resp.extend(sid.to_bytes(4,'little'))
        _resp.extend((1).to_bytes(4,'little'))
        _resp.extend((0).to_bytes(4,'little'))
        logger.info("Ping received.")
        return _resp

    def _info(self, flags: int, idle_rate: int, err_data: bytes):
        # Print info: sdsFlags, sdsIdleRate, Error
        _resp = bytearray()
        self._flags.update_from_target(flags)
        self._request_auto_playback_if_needed(flags)
        if self._info_flags != flags:
            logger.info(f"sdsFlags = 0x{flags:08X}.")
            self._info_flags = flags
        if idle_rate and self._info_IdleRate != idle_rate:
            if idle_rate != 0xFFFFFFFF:
                logger.info(f"{idle_rate}% idle.")
            self._info_IdleRate = idle_rate
        if err_data:
            _status = int.from_bytes(err_data[0:4],'little')
            _line   = int.from_bytes(err_data[4:8],'little')
            _err_mgs = err_data[8:]
            if _status == 0:
                 _msg = "Error:    SDS_ASSERT failed: "
            else:
                _msg = f"Error:    SDS_ERROR_CHECK {_status}: "
            _msg += f"{_err_mgs.decode('utf-8', errors='replace')}: {_line}"
            logger.info(_msg)

        if self._monitor:
            self._monitor.send_info_msg(flags, idle_rate, err_data)

        return _resp

    def clean(self):
        # close all open streams
        for _sid in list(self.opened_streams.keys()):
            self._close(_sid)

    def _get_async_flags(self):
        _resp = bytearray()
        _cmd = CMD_FLAGS
        _set_mask   = self._flags.consume_set()
        _clear_mask = self._flags.consume_clear()
        _resp.extend(_cmd.to_bytes(4,'little'))
        _resp.extend(_set_mask.to_bytes(4,'little'))
        _resp.extend(_clear_mask.to_bytes(4,'little'))
        _resp.extend((0).to_bytes(4,'little'))
        return _resp

    def get_async_response(self):
        _now = time.time()
        if _now - self._last_async_time >= 0.1:
            self._last_async_time = _now
            return self._get_async_flags()
        return None

    def get_shutdown_flags(self):
        _resp = bytearray()
        _cmd = CMD_FLAGS
        _resp.extend(_cmd.to_bytes(4,'little'))
        _resp.extend((0).to_bytes(4,'little'))
        _resp.extend((1 << 28).to_bytes(4,'little'))
        _resp.extend((0).to_bytes(4,'little'))
        return _resp

    def execute_request(self, buf: bytes):
        _cmd = int.from_bytes(buf[0:4],'little')
        if _cmd in CMD_SYNC:
            _sid = int.from_bytes(buf[4:8],'little')
            _arg = int.from_bytes(buf[8:12],'little')
            _sz  = int.from_bytes(buf[12:16],'little')
            _data= buf[16:16+_sz]
            if   _cmd == CMD_OPEN:  return self._open(_arg, _data.decode('utf-8').rstrip('\0'))
            elif _cmd == CMD_CLOSE: return self._close(_sid)
            elif _cmd == CMD_WRITE: return self._write(_sid, _data)
            elif _cmd == CMD_READ:  return self._read(_sid, _arg)
            elif _cmd == CMD_PING:  return self._pingServer(_sid)
        elif _cmd == CMD_INFO:
            _flags     = int.from_bytes(buf[4:8],'little')
            _idle_rate = int.from_bytes(buf[8:12],'little')
            _err_len   = int.from_bytes(buf[12:16],'little')
            _err_data= buf[16:16+_err_len]
            return self._info(_flags, _idle_rate, _err_data)

        else:
            logger.error(f"=== FATAL ERROR === : Data integrity error - protocol mismatch. Restart the SDSIO-Client.")
            return bytearray()


# ---------------------------------------------------------------------------- #
#                            Async Socket Server                               #
# ---------------------------------------------------------------------------- #
class async_sdsio_server_socket:
    def __init__(self, ip, port, connect_mode, connect_message, connect_time_ms, manager: sdsio_manager):
        self._ip = ip
        self._port = port
        self._connect_mode = connect_mode
        self._connect_message = connect_message
        self._connect_time_ms = connect_time_ms
        self._manager = manager
        self.server = None
        self._active_writer = None
        self._handler_tasks = set()
        self._shutting_down = False

    async def _safe_close(self, reader, writer):
        writer.close()
        try:
            await writer.wait_closed()
        except Exception:
            pass
        try:
            reader.feed_eof()
        except Exception:
            pass

    async def _discard_initial_response(self, reader: asyncio.StreamReader, timeout_ms=50):
        _deadline = asyncio.get_running_loop().time() + (timeout_ms / 1000)
        while True:
            _remaining = _deadline - asyncio.get_running_loop().time()
            if _remaining <= 0:
                break
            try:
                _data = await asyncio.wait_for(reader.read(4096), timeout=_remaining)
                if _data:
                    logger.debug(f"Discarding initial response data from host: {_data!r}.")
            except asyncio.TimeoutError:
                break
            if not _data:
                break

    async def _handle_connection(self, reader: asyncio.StreamReader, writer: asyncio.StreamWriter):
        _task = asyncio.current_task()
        self._handler_tasks.add(_task)
        self._active_writer = writer
        try:
            logger.info("SDSIO-Client connected.")
            while True:
                # Send async FLAGS response periodically
                _resp = self._manager.get_async_response()
                if _resp:
                    writer.write(_resp)
                    await writer.drain()

                try:
                    # read fixed-size header, then payload, with a timeout
                    _hdr = await asyncio.wait_for(reader.readexactly(16), timeout=0.1)
                except asyncio.TimeoutError:
                    continue # No data from client, loop to check for FLAGS send

                # validate command before reading payload
                _cmd = int.from_bytes(_hdr[0:4],'little')
                if _cmd not in CMD_ALL:
                    logger.error(f"=== FATAL ERROR === : Data integrity error - protocol mismatch. Restart the SDSIO-Client.")
                    logger.info("Closing SDSIO-Client connection...")
                    break

                _sz  = int.from_bytes(_hdr[12:16],'little')
                _pl  = await reader.readexactly(_sz) if _sz > 0 else b''
                _resp= self._manager.execute_request(_hdr + _pl)
                if _resp:
                    writer.write(_resp)
                    await writer.drain()
        except asyncio.CancelledError:
            raise                          # re-raise per docs
        except (asyncio.IncompleteReadError, ConnectionResetError, OSError):
            if not self._shutting_down:
                logger.info("SDSIO-Client disconnected.")
        finally:
            self._handler_tasks.discard(_task)
            if self._active_writer is writer:
                self._active_writer = None
            # Send shutdown flags (clear alive bit) before closing
            try:
                writer.write(self._manager.get_shutdown_flags())
                await writer.drain()
            except BaseException:
                pass
            await self._safe_close(reader, writer)
            # Clean up any streams
            self._manager.clean()
            if not self._shutting_down:
                if not self._connect_mode:
                    logger.info("SDSIO-Server waiting for SDSIO-Client to reconnect...")

    async def start(self):
        if self._connect_mode:
            await self._start_client()
            return

        await self._start_server()

    async def _start_client(self):
        try:
            _log_connection_attempt = True
            while True:
                _writer = None
                try:
                    if _log_connection_attempt:
                        logger.info(f"SDSIO-Server connecting to socket host {self._ip}:{self._port}...")
                    _reader, _writer = await asyncio.open_connection(self._ip, self._port)
                    _log_connection_attempt = True
                    if self._connect_message is not None:
                        _writer.write(str(self._connect_message).encode("utf-8"))
                        await _writer.drain()
                        await self._discard_initial_response(_reader, self._connect_time_ms)
                    await self._handle_connection(_reader, _writer)
                except (ConnectionRefusedError, TimeoutError, OSError):
                    if _writer:
                        _writer.close()
                        try:
                            await _writer.wait_closed()
                        except Exception:
                            pass
                    if self._shutting_down:
                        break
                    _log_connection_attempt = False
                    self._manager.clean()
                    await asyncio.sleep(1)
                else:
                    if self._shutting_down:
                        break
                    _log_connection_attempt = True
                    await asyncio.sleep(1)
        except KeyboardInterrupt:
            # Python 3.9-3.10: KeyboardInterrupt raised directly (not as CancelledError)
            pass
        finally:
            self._shutting_down = True
            if self._active_writer:
                self._active_writer.close()
                try:
                    await self._active_writer.wait_closed()
                except Exception:
                    pass
                self._active_writer = None

    async def _start_server(self):
        self.server = await asyncio.start_server(self._handle_connection, self._ip, self._port)
        _addr = self.server.sockets[0].getsockname()
        logger.info(f"SDSIO-Server listening on {_addr[0]}:{_addr[1]}...")
        try:
            # Block until cancelled (start_server already accepts connections)
            await asyncio.Event().wait()
        except KeyboardInterrupt:
            # Python 3.9-3.10: KeyboardInterrupt raised directly (not as CancelledError)
            pass
        finally:
            # Signal handlers that this is a deliberate shutdown
            self._shutting_down = True
            # Stop accepting new connections
            self.server.close()
            # Cancel active handler tasks so they don't block shutdown
            for _task in list(self._handler_tasks):
                _task.cancel()
            if self._handler_tasks:
                try:
                    _done, _pending = await asyncio.wait(
                        list(self._handler_tasks), timeout=2.0
                    )
                except BaseException:
                    _pending = self._handler_tasks
                for _task in _pending:
                    _task.cancel()

async def sdsio_server_socket_run_supervised(ip, port, connect_mode, connect_message, connect_time_ms, manager):
    while True:
        _srv = async_sdsio_server_socket(ip, port, connect_mode, connect_message, connect_time_ms, manager)
        try:
            await _srv.start()
            # If start() returns normally, break out
            logger.info("SDSIO-Server shut down cleanly.")
            break
        except Exception:
            logger.info("SDSIO-Server fatal error.")
            logger.info("SDSIO-Server restarting...")
            # Clean up any streams
            manager.clean()
            # Close the listener socket if it’s open
            if _srv.server:
                _srv.server.close()
                await _srv.server.wait_closed()
            await asyncio.sleep(1)


# ---------------------------------------------------------------------------- #
#                           Blocking Serial Server                             #
# ---------------------------------------------------------------------------- #
class sdsio_server_serial:
    def __init__(self, port, baudrate, parity, stop_bits, connect_timeout, manager: sdsio_manager):
        self._port = port
        self._baudrate = baudrate
        self._parity = parity
        self._stop_bits = stop_bits
        self._connect_timeout = connect_timeout
        self._manager = manager
        self._ser = None

    def _open(self):
        _first_attempt = True
        try:
            self._ser = serial.Serial()
            if sys.platform != "darwin":
                if 'COM' in self._port:
                    self._ser.port = self._port
                else:
                    self._ser.port = f"COM{self._port}"
            else:
                self._ser.port = f"/dev/tty{self._port}"
            self._ser.baudrate = self._baudrate
            self._ser.parity = self._parity
            self._ser.stopbits = self._stop_bits
            self._ser.timeout = 0
        except Exception:
            logger.info("Error initializing serial.")
            sys.exit(1)
        _start_time = time.time()
        while not self._manager.shutdown_requested.is_set():
            try:
                self._ser.open()
                logger.info("Serial port opened successfully.")
                return True
            except Exception:
                if _first_attempt:
                    logger.info(f"SDSIO-Server waiting for serial SDSIO-Client on {self._port}...")
                    _first_attempt = False
                if self._connect_timeout and (time.time() - _start_time >= self._connect_timeout):
                    logger.info(f"Serial port open failed after {self._connect_timeout} seconds.")
                    sys.exit(1)
                time.sleep(0.5)
        return False

    def close(self):
        try:
            self._ser.close()
        except Exception:
            pass

    def _read(self, size):
        try:
            return self._ser.read(size)
        except Exception:
            logger.info("Serial read error.")
            raise

    def _write(self, data):
        try:
            return self._ser.write(data)
        except Exception:
            logger.info("Serial write error.")
            raise

    def start(self):
        if not self._open():
            return
        logger.info("Serial Server started.")
        _buffer = bytearray()

        try:
            while not self._manager.shutdown_requested.is_set():
                # Send async FLAGS response periodically
                _resp = self._manager.get_async_response()
                if _resp:
                    self._write(_resp)

                _data = self._read(16 * 1024)
                if _data:
                    _buffer.extend(_data)
                else:
                    time.sleep(0.001)
                    continue

                # Process complete messages from the buffer...
                while len(_buffer) >= 16:
                    _header = _buffer[:16]

                    # validate command before reading payload
                    _cmd = int.from_bytes(_header[0:4], 'little')
                    if _cmd not in CMD_ALL:
                        logger.error(f"=== FATAL ERROR === : Data integrity error - protocol mismatch. Restart the SDSIO-Client.")
                        _buffer.clear()
                        return  # Exit start(), finally block will clean up

                    _data_size = int.from_bytes(_header[12:16], 'little')
                    _req_len = 16 + _data_size
                    if len(_buffer) < _req_len:
                        break
                    _request_buf = _buffer[:_req_len]
                    _buffer = _buffer[_req_len:]
                    _response = self._manager.execute_request(_request_buf)
                    if _response:
                        self._write(_response)
        finally:
            # Send shutdown flags (clear alive bit) before closing
            try:
                self._write(self._manager.get_shutdown_flags())
            except Exception:
                pass
            # Clean up all SDS streams on disconnect/error
            self._manager.clean()
            self._ser.close()

def sdsio_server_serial_run_supervised(port, baudrate, parity, stop_bits, connect_timeout, manager):
    while not manager.shutdown_requested.is_set():
        try:
            _srv = sdsio_server_serial(
                port, baudrate, parity,
                stop_bits, connect_timeout, manager
            )
            _srv.start()
            if manager.shutdown_requested.is_set():
                break
            # start() returned normally (e.g., invalid command)
            logger.info("SDSIO-Server restarting...")
        except Exception:
            if manager.shutdown_requested.is_set():
                break
            logger.info("SDSIO-Server fatal error.")
            logger.info("SDSIO-Server restarting...")
            # Reset all open streams
            manager.clean()
            # Try to close the port if it's still open
            try:
                _srv.close()
            except Exception:
                pass
            time.sleep(1)
        time.sleep(0.1)


# ---------------------------------------------------------------------------- #
#                       Async USB-Bulk Server (usb1)                           #
# ---------------------------------------------------------------------------- #
class sdsio_server_usb:
    _PRODUCT_STR = "SDSIO-Client"
    _XFER_NUM    = 32
    _XFER_SIZE   = 8 * 1024  # 8 KiB per transfer

    # Win32 constants for best-effort priority tuning
    if os.name == "nt":
        _KERNEL32 = ctypes.WinDLL("kernel32", use_last_error=True)
        _KERNEL32.GetCurrentProcess.restype       = ctypes.wintypes.HANDLE
        _KERNEL32.GetCurrentThread.restype        = ctypes.wintypes.HANDLE
        _KERNEL32.SetPriorityClass.argtypes       = (ctypes.wintypes.HANDLE, ctypes.wintypes.DWORD)
        _KERNEL32.SetPriorityClass.restype        = ctypes.wintypes.BOOL
        _KERNEL32.SetThreadPriority.argtypes      = (ctypes.wintypes.HANDLE, ctypes.wintypes.INT)
        _KERNEL32.SetThreadPriority.restype       = ctypes.wintypes.BOOL
        _KERNEL32.SetThreadAffinityMask.argtypes  = (ctypes.wintypes.HANDLE, ctypes.wintypes.DWORD)
        _KERNEL32.SetThreadAffinityMask.restype   = ctypes.wintypes.DWORD
        _PROCESS_REALTIME              = 0x00000100
        _THREAD_PRIORITY_TIME_CRITICAL = 15

    def __init__(self, manager: 'sdsio_manager', loop: asyncio.AbstractEventLoop, high_priority: bool = False):
        self._mgr             = manager
        self._loop            = loop
        self._high_priority   = high_priority
        self._ctx             = None
        self._handle          = None
        self._in_ep           = None
        self._out_ep          = None
        self._in_transfers    = []
        self._out_pool        = []
        self._out_in_flight   = set()
        self._in_q            = asyncio.Queue()
        self._out_q           = asyncio.Queue()
        self._running         = False
        self._rx_buf         = bytearray()
        self._poll_thread    = None
        self._monitor_thread = None
        self._vendor_id       = None
        self._product_id      = None
        self._protocol_error = False
        self._shutdown_event = threading.Event()
        self._disconnect_lock = threading.Lock()

    async def _open(self):
        _first_attempt = True
        try:
            _usb_dev = None
            while _usb_dev is None:
                for _dev in self._ctx.getDeviceList(skip_on_error=True):
                    try:
                        if _dev.getProduct() == self._PRODUCT_STR:
                            _usb_dev = _dev
                            break
                    except usb1.USBError:
                        continue
                if _usb_dev is None:
                    if _first_attempt:
                        logger.info("SDSIO-Server waiting for USB SDSIO-Client...")
                        _first_attempt = False
                    await asyncio.sleep(0.5)

            # remember IDs for reconnect detection
            self._vendor_id  = _usb_dev.getVendorID()
            self._product_id = _usb_dev.getProductID()

            self._handle = _usb_dev.open()
            try:
                if self._handle.kernelDriverActive(0):
                    self._handle.detachKernelDriver(0)
            except Exception:
                pass
            self._handle.claimInterface(0)

            # discover bulk endpoints
            for _cfg in _usb_dev.iterConfigurations():
                for _intf in _cfg.iterInterfaces():
                    for _sett in _intf.iterSettings():
                        for _ep in _sett.iterEndpoints():
                            if (_ep.getAttributes() & usb1.TRANSFER_TYPE_MASK) != usb1.TRANSFER_TYPE_BULK:
                                continue
                            _addr = _ep.getAddress()
                            if _addr & usb1.ENDPOINT_DIR_MASK:
                                self._in_ep  = _addr
                            else:
                                self._out_ep = _addr

            if not (self._in_ep and self._out_ep):
                raise RuntimeError("Bulk endpoints not found.")

        except asyncio.CancelledError:
            raise

    def _tune_current_thread(self):
        """Best-effort real-time tuning."""
        try:
            if os.name == "nt":
                _hProc = self._KERNEL32.GetCurrentProcess()
                self._KERNEL32.SetPriorityClass(_hProc, self._PROCESS_REALTIME)
                _hThr = self._KERNEL32.GetCurrentThread()
                self._KERNEL32.SetThreadPriority(_hThr, self._THREAD_PRIORITY_TIME_CRITICAL)
                self._KERNEL32.SetThreadAffinityMask(_hThr, 1)
            else:
                _libc_name = "libc.so.6" if sys.platform.startswith("linux") else "libc.dylib"
                _libc = ctypes.CDLL(_libc_name, use_errno=True)
                _SCHED_FIFO = 1
                class sched_param(ctypes.Structure):
                    _fields_ = [("sched_priority", ctypes.c_int)]
                _max_prio = _libc.sched_get_priority_max(_SCHED_FIFO)
                _param    = sched_param(_max_prio)
                _tid      = _libc.pthread_self()
                _libc.pthread_setschedparam(_tid, _SCHED_FIFO, ctypes.byref(_param))
                try:
                    os.nice(-20)
                except OSError:
                    pass
        except Exception:
            logger.exception("[TUNE] thread tuning failed.")

    def _libusb_loop(self):
        if self._high_priority:
            self._tune_current_thread()
        while self._running:
            try:
                self._ctx.handleEventsTimeout(tv=0)
            except usb1.USBErrorInterrupted:
                pass

    def _signal_disconnect(self):
        """Thread-safe: stop session and wake coroutines.

        Does NOT print any message — the caller in start() determines
        whether the device is truly gone after gather() exits.
        """
        with self._disconnect_lock:
            if not self._running:
                return                          # already handled
            self._running = False
        try:
            self._loop.call_soon_threadsafe(self._in_q.put_nowait,  b'')
            self._loop.call_soon_threadsafe(self._out_q.put_nowait, b'')
        except Exception:
            pass

    def _on_in_complete(self, xfer: usb1.USBTransfer):
        _status = xfer.getStatus()
        if _status == usb1.TRANSFER_COMPLETED:
            _data = bytes(xfer.getBuffer()[:xfer.getActualLength()])
            self._loop.call_soon_threadsafe(self._in_q.put_nowait, _data)
        if self._running:
            try:
                xfer.submit()
            except usb1.USBError as _e:
                # Track dead transfers; if all IN transfers fail, trigger reconnect
                self._dead_in_xfers.add(id(xfer))
                if len(self._dead_in_xfers) >= len(self._in_transfers):
                    self._signal_disconnect()

    def _on_out_complete(self, xfer: usb1.USBTransfer):
        self._out_in_flight.discard(xfer)
        self._out_pool.append(xfer)

    def _on_hotplug(self, context, device, event):
        if event & usb1.HOTPLUG_EVENT_DEVICE_LEFT:
            self._signal_disconnect()

    def _monitor_loop(self):
        _miss_count = 0
        _MISS_THRESHOLD = 3          # require 3 consecutive misses (~1.5 s)
        _poll_ctx = usb1.USBContext()
        try:
            while self._running:
                time.sleep(0.5)
                _found = False
                for _dev in _poll_ctx.getDeviceList(skip_on_error=True):
                    try:
                        if (_dev.getVendorID()  == self._vendor_id and
                            _dev.getProductID() == self._product_id):
                            _found = True
                            break
                    except usb1.USBError:
                        continue
                if _found:
                    _miss_count = 0
                else:
                    _miss_count += 1
                    if _miss_count >= _MISS_THRESHOLD:
                        self._signal_disconnect()
                        break
        finally:
            try:
                _poll_ctx.close()
            except Exception:
                pass

    async def _consumer(self):
        while self._running:
            # Send async FLAGS response periodically
            _resp = self._mgr.get_async_response()
            if _resp:
                await self._out_q.put(_resp)

            try:
                _data = await asyncio.wait_for(self._in_q.get(), timeout=0.1)
            except asyncio.TimeoutError:
                continue  # No data from device, loop to check for FLAGS send

            self._rx_buf.extend(_data)
            while len(self._rx_buf) >= 16:
                _hdr   = self._rx_buf[:16]

                # validate command before reading payload
                _cmd = int.from_bytes(_hdr[0:4],'little')
                if _cmd not in CMD_ALL:
                    logger.error(f"=== FATAL ERROR === : Data integrity error - protocol mismatch. Restart the SDSIO-Client.")
                    self._protocol_error = True
                    self._running = False
                    self._rx_buf.clear()
                    # Wake up out_sender so it can exit cleanly
                    self._out_q.put_nowait(b'')
                    break

                _sz    = int.from_bytes(_hdr[12:16], 'little')
                _total = 16 + _sz
                if len(self._rx_buf) < _total:
                    break
                _payload = bytes(self._rx_buf[16:_total])
                del self._rx_buf[:_total]
                _resp = self._mgr.execute_request(_hdr + _payload)
                if _resp:
                    await self._out_q.put(_resp)
            self._in_q.task_done()

    async def _out_sender(self):
        while self._running or not self._out_q.empty():
            _resp = await self._out_q.get()
            # wait for an available OUT-URB
            while self._running and not self._out_pool:
                await asyncio.sleep(0)
            if not self._out_pool:
                break
            _xfer = self._out_pool.pop()
            try:
                _xfer.setBulk(self._out_ep, _resp,
                             callback=self._on_out_complete, timeout=0)
                _xfer.submit()
                self._out_in_flight.add(_xfer)
            except usb1.USBError:
                break
            finally:
                self._out_q.task_done()

    async def start(self):
        _silent_reconnect = False
        while True:
            self._ctx = usb1.USBContext()
            self._handle          = None
            self._in_ep           = None
            self._out_ep          = None
            self._in_transfers    = []
            self._out_pool        = []
            self._out_in_flight   = set()
            self._in_q            = asyncio.Queue()
            self._out_q           = asyncio.Queue()
            self._running         = False
            self._rx_buf         = bytearray()
            self._poll_thread    = None
            self._monitor_thread = None
            self._vendor_id       = None
            self._product_id      = None
            self._protocol_error = False
            self._dead_in_xfers  = set()
            self._shutdown_event.clear()

            try:
                await self._open()
            except asyncio.CancelledError:
                # Ctrl+C while waiting for device — close USB context to avoid leak
                try:
                    self._ctx.close()
                except Exception:
                    pass
                raise

            # hotplug if possible...
            try:
                self._ctx.hotplugRegisterCallback(
                    callback   = self._on_hotplug,
                    events     = usb1.HOTPLUG_EVENT_DEVICE_LEFT,
                    flags      = 0,
                    vendor_id  = 0,
                    product_id = 0,
                    dev_class  = usb1.HOTPLUG_MATCH_ANY
                )
            except (AttributeError, usb1.USBError):
                # Hotplug not available; use polling monitor thread
                self._running = True
                self._monitor_thread = threading.Thread(
                    target=self._monitor_loop,
                    name="usb-monitor",
                    daemon=True
                )
                self._monitor_thread.start()

            # prepare OUT‐URB pool
            for _ in range(self._XFER_NUM):
                _x = self._handle.getTransfer()
                _x.setBulk(self._out_ep, bytearray(self._XFER_SIZE),
                          callback=self._on_out_complete, timeout=0)
                self._out_pool.append(_x)

            # fire IN‐URBs
            for _ in range(self._XFER_NUM):
                _x = self._handle.getTransfer()
                _x.setBulk(self._in_ep, bytearray(self._XFER_SIZE),
                          callback=self._on_in_complete, timeout=0)
                if hasattr(_x, "setShortNotOk"):
                    _x.setShortNotOk(True)
                _x.submit()
                self._in_transfers.append(_x)

            # Mark server active in the normal path too (hotplug working)
            self._running = True
            if not _silent_reconnect:
                logger.info(f"SDSIO-Client USB device connected.")
            else:
                logger.debug("USB connection re-established.")
                _silent_reconnect = False

            # start polling thread
            self._poll_thread = threading.Thread(
                target=self._libusb_loop, name="usb-poll", daemon=True
            )
            self._poll_thread.start()

            # run until disconnect or until _close() flips self._running=False
            try:
                await asyncio.gather(self._consumer(), self._out_sender())
            except (asyncio.CancelledError):
                # Send shutdown flags (clear alive bit) before cleanup
                try:
                    self._handle.bulkWrite(self._out_ep, self._mgr.get_shutdown_flags(), timeout=1000)
                except Exception:
                    pass
                self._mgr.clean()
                self._close()
                raise

            # Send shutdown flags (clear alive bit) before cleanup
            try:
                self._handle.bulkWrite(self._out_ep, self._mgr.get_shutdown_flags(), timeout=1000)
            except Exception:
                pass

            # clean up this connection
            _protocol_err = self._protocol_error
            self._mgr.clean()
            self._close()

            # Determine whether the device is truly gone (using a fresh context
            # so we don't rely on the now-closed self._ctx).
            if not _protocol_err:
                _check_ctx = usb1.USBContext()
                try:
                    _device_present = any(
                        _dev for _dev in _check_ctx.getDeviceList(skip_on_error=True)
                        if (_dev.getVendorID()  == self._vendor_id and
                            _dev.getProductID() == self._product_id)
                    )
                except Exception:
                    _device_present = False
                finally:
                    try:
                        _check_ctx.close()
                    except Exception:
                        pass

                if _device_present:
                    # Transient USB failure — device is still plugged in.
                    # Loop back and silently reconnect.
                    logger.debug("USB transfers interrupted; reconnecting...")
                    _silent_reconnect = True
                    continue
                else:
                    logger.info("USB SDSIO-Client disconnected.")

            # On protocol error, wait for device to be physically reset
            if _protocol_err:
                try:
                    self._shutdown_event.clear()
                    await asyncio.to_thread(self._wait_for_device_reset)
                except asyncio.CancelledError:
                    # Ctrl+C during device reset wait
                    try:
                        self._shutdown_event.set()
                    except Exception:
                        pass
                    raise

    def _wait_for_device_reset(self):
        """Wait for USB device to be removed and reattached."""
        _vid = self._vendor_id
        _pid = self._product_id
        if not _vid or not _pid:
            return

        _poll_ctx = usb1.USBContext()
        try:
            def _device_present():
                for _dev in _poll_ctx.getDeviceList(skip_on_error=True):
                    try:
                        if _dev.getVendorID() == _vid and _dev.getProductID() == _pid:
                            return True
                    except usb1.USBError:
                        continue
                return False

            # Wait for device to disappear
            logger.info("SDSIO-Server waiting for USB SDSIO-Client device to disconnect...")
            while _device_present():
                if self._shutdown_event.wait(0.5):
                    return

            # Wait for device to reappear
            logger.info("SDSIO-Server waiting for USB SDSIO-Client device to reconnect...")
            while not _device_present():
                if self._shutdown_event.wait(0.5):
                    return
        finally:
            try:
                _poll_ctx.close()
            except Exception:
                pass

    def _close(self):
        # Called either by KeyboardInterrupt or by internal errors/disconnects
        self._running = False
        self._shutdown_event.set()

        # Ensure any asyncio coroutines blocked on the queues are woken
        try:
            self._loop.call_soon_threadsafe(self._in_q.put_nowait, b'')
            self._loop.call_soon_threadsafe(self._out_q.put_nowait, b'')
        except Exception:
            pass

        for _x in self._in_transfers:
            try:
                _x.cancel()
            except:
                pass
        self._in_transfers.clear()

        for _x in list(self._out_in_flight):
            try:
                _x.cancel()
            except:
                pass
        self._out_in_flight.clear()

        for _x in self._out_pool:
            try:
                _x.cancel()
            except:
                pass
        self._out_pool.clear()

        try:
            self._ctx.handleEventsTimeout(tv=0)
        except:
            pass

        try:
            self._handle.releaseInterface(0)
        except:
            pass
        try:
            self._handle.close()
        except:
            pass
        try:
            self._ctx.close()
        except:
            pass

        if self._poll_thread:
            self._poll_thread.join(timeout=2.0)
        if self._monitor_thread:
            self._monitor_thread.join(timeout=2.0)

async def sdsio_server_usb_run_supervised(manager):
    _loop = asyncio.get_running_loop()
    _srv  = sdsio_server_usb(manager, _loop)
    await _srv.start()


# ---------------------------------------------------------------------------- #
#                        Argument Parsing & Entry Point                        #
# ---------------------------------------------------------------------------- #
def dir_path(work_dir):
    if path.isdir(work_dir):
        return work_dir
    else:
        raise argparse.ArgumentTypeError(f"Directory '{work_dir}' does not exist!")

def log_file_path(log_file):
    _log_dir = path.dirname(path.abspath(log_file))
    if path.isdir(_log_dir):
        return log_file
    else:
        raise argparse.ArgumentTypeError(f"Directory '{_log_dir}' for log file does not exist!")

def non_negative_int(value):
    try:
        _value = int(value)
    except ValueError:
        raise argparse.ArgumentTypeError(f"Invalid integer value: {value}!")
    if _value < 0:
        raise argparse.ArgumentTypeError("Value must be 0 or greater!")
    return _value

def ip_validator(ip_str):
    try:
        ipaddress.ip_address(ip_str)
        return ip_str
    except:
        raise argparse.ArgumentTypeError(f"Invalid IP address: {ip_str}!")

def interface_validator(interface):
    try:
        _adapters = ifaddr.get_adapters()
        for _adapter in _adapters:
            _name = _adapter.name.replace('{', '').replace('}', '')
            _nice_name = _adapter.nice_name.replace('{', '').replace('}', '')
            if _name == interface or _nice_name == interface:
                return _name
    except:
        pass
    raise argparse.ArgumentTypeError(f"Invalid network interface: {interface}!")

def parse_arguments():
    # Custom parser with targeted epilog behavior
    class MSStyleArgumentParser(argparse.ArgumentParser):
        def __init__(self, *args, **kwargs):
            super().__init__(*args, **kwargs)
            self.is_top_level = False
            self.is_subparser = False
            self._ms_show_epilog_once = False   # only for "missing server type"
            self.error_hint = ""                # printed ONLY on subparser errors

        def error(self, message):
            # Always show the relevant usage
            self.print_usage(sys.stderr)

            # Decide what footer to show
            _epilog = ""
            # Only show the top-level epilog if we explicitly asked for it (missing server type)
            if self.is_top_level and self._ms_show_epilog_once and self.epilog:
                try:
                    _epilog_text = self.epilog % {"prog": self.prog}
                except Exception:
                    _epilog_text = self.epilog
                _epilog = "\n\n" + _epilog_text + "\n"

            # For subparser errors, append a concise per-server hint (not epilog, so help -h stays clean)
            _hint = ""
            if self.is_subparser and self.error_hint:
                try:
                    _hint = "\n\n" + (self.error_hint % {"prog": self.prog}) + "\n"
                except Exception:
                    _hint = "\n\n" + self.error_hint + "\n"

            # Reset one-shot flag
            self._ms_show_epilog_once = False

            self.exit(2, f"{self.prog}: error: {message}{_epilog}{_hint}")

        def error_with_epilog(self, message):
            self._ms_show_epilog_once = True
            self.error(message)

    class SdsFormatter(argparse.RawTextHelpFormatter):
        def __init__(self, prog, **kwargs):
            super().__init__(prog, max_help_position=41, **kwargs)
        def _format_action_invocation(self, action):
            if not action.option_strings or action.nargs == 0:
                return super()._format_action_invocation(action)
            _opts = ', '.join(action.option_strings)
            if action.metavar:
                return f"{_opts} {action.metavar}"
            return _opts
    _formatter = lambda prog: SdsFormatter(prog)

    # Top-level epilog shown with -h (keyboard input, examples, server type help)
    _top_epilog = (
        "keyboard input (while running):\n"
        "  R/r  start recording      P/p  start playback\n"
        "  S/s  stop rec/play        X/x  terminate server\n"
        "  A-H  set user flags 0-7   a-h  clear user flags 0-7\n"
        "  T/t  reset target\n"
        "\n"
        "examples:\n"
        "  %(prog)s -c sdsio.yml                     # Recommended: all config in YAML\n"
        "  %(prog)s -c sdsio.yml --playback          # Playback mode\n"
        "  %(prog)s -c sdsio.yml --mon-port 6060     # With VS Code SDS extension monitor\n"
        "  %(prog)s usb --workdir ./data             # USB server, explicit work dir\n"
        "  %(prog)s socket --port 5050               # TCP socket server\n"
        "  %(prog)s serial --port COM3 --baudrate 115200\n"
        "\n"
        "server type help:\n"
        "  %(prog)s socket -h\n"
        "  %(prog)s serial -h\n"
        "  %(prog)s usb -h\n"
    )

    # top-level parser
    _parser = MSStyleArgumentParser(
        formatter_class=_formatter,
        add_help=False,
        description=(
            "SDSIO-Server: record and playback SDS data stream files over USB, socket, or serial interface.\n"
            "Configure via *.sdsio.yml file or specify the interface parameters directly on the command line."
        ),
        epilog=_top_epilog,
    )
    _parser.is_top_level = True
    _parser.usage = "%(prog)s [-h] [-V] [{socket | serial | usb} [if-opts]] [-c sdsio.yml] [general-opts]"

    def _add_info_opts(p, version_text=None):
        _options = p.add_argument_group("options")
        _options.add_argument("--help", "-h", help="Show this help message and exit", action="help")
        _options.add_argument("--version", "-V", action="version",
                              help="Show program's version number and exit",
                              version=version_text or f"%(prog)s {SDSIO_SERVER_VERSION}")

    _add_info_opts(_parser)

    # Subparsers
    _subparsers = _parser.add_subparsers(
        dest="server_type",
        title="interface (optional, default: usb; overrides interface specified in *.sdsio.yml)",
        metavar="{socket | serial | usb}",
        parser_class=MSStyleArgumentParser
    )

    _configuration = _parser.add_argument_group("configuration")
    _configuration.add_argument("--control", "-c", dest="ctrl_yml", metavar="<*.sdsio.yml>",
                        help="Configure interface, SDS file directories, and playback steps", required=False)

    def _add_general_opts(p):
        """Append a general-opts group to subparser p (called after server-specific groups)."""
        _g = p.add_argument_group("general-opts")
        _g.add_argument("--playback", "-p", dest="auto_playback", action="store_true",
                       help="Start SDSIO-Server in playback mode (typically used in CI tests)",
                       default=argparse.SUPPRESS)
        _g.add_argument("--workdir", dest="work_dir", metavar="<path>",
                       help="Directory for SDS files (overrides *.sdsio.yml setting; default: current directory)",
                       type=dir_path, default=argparse.SUPPRESS)
        _g.add_argument("--mon-port", "-m", dest="monitor_port", metavar="<port>",
                       help="Monitor control interface port", type=int, default=argparse.SUPPRESS)
        _g.add_argument("--log", "-l", dest="log_file", metavar="<file>",
                       help="Redirect console output to a log file (typically for CI use)",
                       type=log_file_path, default=argparse.SUPPRESS)
        _g.add_argument("--write-flush-size", dest="write_flush_size", metavar="<bytes>",
                       help="Force recorded SDS data to disk after this many bytes (0 disables explicit sync)",
                       type=non_negative_int, default=argparse.SUPPRESS)
        _g.add_argument("--verbose", "-v", action="store_true",
                       help="Enable debug messages", default=argparse.SUPPRESS)
        _g.add_argument("--high-priority", dest="high_priority",
                       help="Increase process priority when using USB interface (requires elevated privileges)",
                       action="store_true", default=argparse.SUPPRESS)

    # socket
    _parser_socket = _subparsers.add_parser(
        "socket",
        prog=f"{_parser.prog} socket",
        formatter_class=_formatter,
        add_help=False,
        help="Run TCP socket server",
        epilog="",  # keep subcommand help clean
    )
    _parser_socket.is_subparser = True
    _parser_socket.error_hint = "For help on how to use the socket server and its arguments, run: %(prog)s -h"
    _parser_socket.usage = "%(prog)s [-h] [-V] [--ipaddr <IP> | --netif <Interface>] [--port <TCP Port>] [--connect-mode] [--connect-message <message>] [--connect-time <ms>] [general-opts]"
    _add_info_opts(_parser_socket, version_text=f"{_parser.prog} {SDSIO_SERVER_VERSION}")

    _socket_group = _parser_socket.add_argument_group("if-opts (optional)")
    _socket_group.add_argument("--ipaddr", dest="ip", metavar="<IP>",
                              help="Server IP address, or host IP address in connect mode (example: 192.168.0.100); mandatory with --connect-mode; cannot be combined with 'netif'",
                              type=ip_validator, default=None)
    _socket_group.add_argument("--netif", dest="interface", metavar="<Interface>",
                              help="Network interface (example: eth0), cannot be combined with 'ipaddr'",
                              type=interface_validator, default=None)
    _socket_group.add_argument("--port", dest="port", metavar="<TCP Port>",
                              help="TCP port number (default: 5050)", type=int, default=5050)
    _socket_group.add_argument("--connect-mode", dest="connect_mode",
                              help="Connect to the configured IP address instead of listening for incoming socket connections",
                              action="store_true", default=False)
    _socket_group.add_argument("--connect-message", dest="connect_message", metavar="<message>",
                              help="Optional message sent when the connect-mode socket connection is established",
                              default=None)
    _socket_group.add_argument("--connect-time", dest="connect_time_ms", metavar="<ms>",
                              help="Duration in milliseconds to discard incoming data after sending connect-message (default: 50)",
                              type=non_negative_int, default=50)
    _add_general_opts(_parser_socket)

    # serial
    _parser_serial = _subparsers.add_parser(
        "serial",
        prog=f"{_parser.prog} serial",
        formatter_class=_formatter,
        add_help=False,
        help="Run serial server",
        epilog="",  # keep subcommand help clean
    )
    _parser_serial.is_subparser = True
    _parser_serial.error_hint = "For help on how to use the serial server and its arguments, run: %(prog)s -h"
    _parser_serial.usage = "%(prog)s [-h] [-V] --port <Serial Port> [--baudrate <Baudrate>] [--parity <Parity>] [--stopbits <Stop bits>] [--connect-timeout <Timeout>] [general-opts]"
    _add_info_opts(_parser_serial, version_text=f"{_parser.prog} {SDSIO_SERVER_VERSION}")

    _serial_required = _parser_serial.add_argument_group("if-opts (required)")
    _serial_required.add_argument("--port", dest="port", metavar="<Serial Port>",
                                 help="Serial port (required)", required=True)

    _serial_optional = _parser_serial.add_argument_group("if-opts (optional)")
    _serial_optional.add_argument("--baudrate", dest="baudrate", metavar="<Baudrate>",
                                 help="Baudrate (default: 115200)", type=int, default=115200)
    _parity_help = "Parity: none, even, odd, mark, space (default: none)"
    _serial_optional.add_argument("--parity", dest="parity", metavar="<Parity>",
                                 choices=list(PARITY_NAME_MAP.keys()), help=_parity_help, default='none')
    _stopbits_help = (f"Stop bits: {serial.STOPBITS_ONE}, {serial.STOPBITS_ONE_POINT_FIVE}, "
                     f"{serial.STOPBITS_TWO} (default: {serial.STOPBITS_ONE})")
    _serial_optional.add_argument("--stopbits", dest="stop_bits", metavar="<Stop bits>",
                                 type=float,
                                 choices=[serial.STOPBITS_ONE, serial.STOPBITS_ONE_POINT_FIVE, serial.STOPBITS_TWO],
                                 help=_stopbits_help, default=serial.STOPBITS_ONE)
    _serial_optional.add_argument("--connect-timeout", dest="connect_timeout", metavar="<Timeout>",
                                 help="Serial port connection timeout in seconds (default: no timeout)",
                                 type=float, default=None)
    _add_general_opts(_parser_serial)

    # usb
    _parser_usb = _subparsers.add_parser(
        "usb",
        prog=f"{_parser.prog} usb",
        formatter_class=_formatter,
        add_help=False,
        help="Run USB bulk server",
        epilog="",  # keep subcommand help clean
    )
    _parser_usb.is_subparser = True
    _parser_usb.error_hint = "For help on how to use the usb server and its arguments, run: %(prog)s -h"
    _parser_usb.usage = "%(prog)s [-h] [-V] [general-opts]"
    _add_info_opts(_parser_usb, version_text=f"{_parser.prog} {SDSIO_SERVER_VERSION}")
    _add_general_opts(_parser_usb)

    _general = _parser.add_argument_group("general-opts")
    _general.add_argument("--playback", "-p", dest="auto_playback", action="store_true",
                         help="Start SDSIO-Server in playback mode (typically used in CI tests)", default=None)
    _general.add_argument("--workdir", dest="work_dir", metavar="<path>",
                        help="Directory for SDS files (overrides *.sdsio.yml setting; default: current directory)", type=dir_path, default=None)
    _general.add_argument("--mon-port", "-m", dest="monitor_port", metavar="<port>",
                        help="Monitor control interface port", type=int, default=None)
    _general.add_argument("--log",     "-l", dest="log_file", metavar="<file>",
                        help="Redirect console output to a log file (typically for CI use)", type=log_file_path, default=None)
    _general.add_argument("--write-flush-size", dest="write_flush_size", metavar="<bytes>",
                        help="Force recorded SDS data to disk after this many bytes (0 disables explicit sync)",
                        type=non_negative_int, default=0)
    _general.add_argument("--verbose", "-v", action="store_true", help="Enable debug messages")
    _general.add_argument("--high-priority", dest="high_priority",
                        help="Increase process priority when using USB interface (requires elevated privileges)", action="store_true", default=False)


    # two-phase parse
    _argv = sys.argv[1:]

    # 1) Parse top-level to get the server_type (don't print epilog here unless missing)
    try:
        _top_ns, _ = _parser.parse_known_args(_argv)
    except SystemExit:
        # top-level parse errors like "invalid choice" -> no epilog (as desired)
        raise

    # Missing server type
    if _top_ns.server_type is None:
        if _top_ns.ctrl_yml is None:
            # Set default server type to "usb" if not specified in CLI or YAML
            _top_ns.server_type = "usb"
    else:
        # 2) Hand EXACT user tokens to the chosen subparser (everything after the server type token)
        try:
            _i = _argv.index(_top_ns.server_type)
        except ValueError:
            # Fallback: if not found for some reason, let subparser try the remainder
            _i = 0
        _sub_args = _argv[_i + 1:]
        _subparser = _subparsers.choices[_top_ns.server_type]

        # parse_known_args so opts already handled by the top-level parser
        # (e.g. -c) placed before the server type token are silently ignored
        _sub_ns, _ = _subparser.parse_known_args(_sub_args)

        # Manual mutual-exclusion for socket so both options appear in one group
        if _top_ns.server_type == "socket":
            if _sub_ns.ip is not None and _sub_ns.interface is not None:
                _subparser.error("options --ipaddr and --netif are mutually exclusive.")
            if _sub_ns.connect_mode and _sub_ns.interface is not None:
                _subparser.error("option --connect-mode cannot be combined with --netif.")
            if _sub_ns.connect_mode and _sub_ns.ip is None:
                _subparser.error("option --connect-mode requires --ipaddr.")

        # Merge namespaces (global + subcommand) for downstream use
        for _k, _v in vars(_sub_ns).items():
            setattr(_top_ns, _k, _v)
    return _top_ns

async def main():
    _args = parse_arguments()

    # configure logging
    if _args.verbose:
        _fmt = logging.Formatter("%(asctime)s [%(threadName)s] %(levelname)s: %(message)s")
        _log, _spinner = setup_logger(level=logging.DEBUG, formatter=_fmt, log_file=_args.log_file)
    else:
        _fmt = logging.Formatter("%(message)s")
        _log, _spinner = setup_logger(level=logging.INFO, formatter=_fmt, log_file=_args.log_file)
    # override globals configured before argument parsing
    global logger, spinner
    logger = _log
    spinner = _spinner
    logger.info("SDSIO-Server v%s", SDSIO_SERVER_VERSION)
    logger.info("Press 'Ctrl+C' or 'X' to exit.")

    # Open and parse the control YAML if provided, and apply any configuration
    # CLI arguments will override YAML settings where applicable (e.g. server type)
    _ctrl_data = {}
    _ctrl_yml_dir = os.getcwd()
    if _args.ctrl_yml:
        _ctrl_yml_path = os.path.abspath(_args.ctrl_yml)
        _ctrl_yml_dir = path.dirname(_ctrl_yml_path)
        try:
            with open(_ctrl_yml_path, 'r') as _yml_file:
                _yml_data = yaml.safe_load(_yml_file)
            if 'sdsio' in _yml_data:
                _ctrl_data = _yml_data['sdsio']
            else:
                raise ValueError("Invalid control YAML.")
        except Exception as _e:
            logger.error(f"Failed to load control YAML: {_e}.")

    # Server type
    if _args.server_type is not None:
        # Server configuration from CLI arguments (overrides YAML)
        _server_type = _args.server_type
        if _server_type == "socket":
            _socket_iface = _args.interface
            _ip = _args.ip
            _port = _args.port
            _connect_mode = _args.connect_mode
            _connect_message = _args.connect_message
            _connect_time_ms = _args.connect_time_ms
        elif _server_type == "serial":
            _port = _args.port
            _baudrate = _args.baudrate
            _parity = PARITY_NAME_MAP[_args.parity]
            _stop_bits = _args.stop_bits
            _connect_timeout = _args.connect_timeout
        elif _server_type == "usb":
            _high_priority = _args.high_priority
    else:
        # Server type from YAML (fallback if not specified in CLI)
        # The interface type is determined by which subnode is present: usb, serial, or socket
        _iface_node = _ctrl_data.get('interface', None)
        if _iface_node and 'serial' in _iface_node:
            _server_type = 'serial'
            _iface_cfg = _iface_node['serial'] or {}
        elif _iface_node and 'socket' in _iface_node:
            _server_type = 'socket'
            _iface_cfg = _iface_node['socket'] or {}
        else:
            _server_type = 'usb'  # default
            _iface_cfg = (_iface_node.get('usb') or {}) if _iface_node else {}
        if _server_type == "socket":
            _socket_iface = _iface_cfg.get('netif', None)
            _ip = _iface_cfg.get('ipaddr', None)
            _port = _iface_cfg.get('port', 5050)
            _connect_mode = _iface_cfg.get('connect_mode', False)
            _connect_message = _iface_cfg.get('connect_message', None)
            _connect_time_ms = non_negative_int(_iface_cfg.get('connect_time', 50))
        elif _server_type == "serial":
            _port = _iface_cfg.get('port')
            _baudrate = _iface_cfg.get('baudrate', 115200)
            _parity = PARITY_NAME_MAP.get(str(_iface_cfg.get('parity', 'none')).lower(), serial.PARITY_NONE)
            _stop_bits = _iface_cfg.get('stopbits', serial.STOPBITS_ONE)
            _connect_timeout = None  # YAML config does not support connect timeout
        elif _server_type == "usb":
            _high_priority = _iface_cfg.get('high_priority', False)

    # Working directory
    if _args.work_dir:
        _work_dir = _args.work_dir
    elif _ctrl_data and _ctrl_data.get('workdir'):
        _work_dir = _ctrl_data.get('workdir')
        if not path.isabs(_work_dir):
            _work_dir = path.join(_ctrl_yml_dir, _work_dir)
        _work_dir = path.normpath(_work_dir)
    else:
        _work_dir = os.getcwd()
    if not path.isdir(_work_dir):
        logger.error(f"Working directory does not exist: {_work_dir}.")
        sys.exit(1)

    # Auto playback
    _auto_playback = _args.auto_playback if _args.auto_playback else False

    # Playback list
    _play_list: Optional[list] = _ctrl_data.get('play', None) if _ctrl_data else None

    _manager = sdsio_manager(work_dir=_work_dir, auto_playback=_auto_playback, play_list=_play_list,
                             mon_port=_args.monitor_port, write_flush_size=_args.write_flush_size)

    try:
        if _server_type == "socket":
            if _connect_mode and _socket_iface:
                logger.error("Socket connect mode cannot use network interface selection; configure ipaddr instead.")
                sys.exit(1)
            if _connect_mode and not _ip:
                logger.error("Socket connect mode requires ipaddr.")
                sys.exit(1)
            if not _connect_mode and not _ip and _socket_iface:
                _adapters = ifaddr.get_adapters()
                for _adapter in _adapters:
                    if _adapter.name == _socket_iface or _adapter.nice_name == _socket_iface:
                        for _ip_info in _adapter.ips:
                            try:
                                socket.inet_pton(socket.AF_INET, _ip_info.ip)
                                _ip = _ip_info.ip
                                break
                            except:
                                continue
                    if _ip:
                        break
            if not _ip:
                _ip = socket.gethostbyname(socket.gethostname())
            await sdsio_server_socket_run_supervised(_ip, _port, _connect_mode, _connect_message, _connect_time_ms, _manager)

        elif _server_type == "serial":
            sdsio_server_serial_run_supervised(_port, _baudrate, _parity,
                                               _stop_bits, _connect_timeout, _manager)

        elif _server_type == "usb":
            _loop = asyncio.get_running_loop()
            _srv = sdsio_server_usb(_manager, _loop, high_priority=_high_priority)
            await _srv.start()

    except (KeyboardInterrupt, asyncio.CancelledError):
        pass
    finally:
        # Shutdown the UI thread and clean-up all SDS streams on exit
        _manager.shutdown()

if __name__ == "__main__":
    try:
        asyncio.run(main())
    except (KeyboardInterrupt, asyncio.CancelledError):
        logger.info("Ctrl+C received, shutting down.")
    logger.info("SDSIO-Server stopped.")
