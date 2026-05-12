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
from pandas import options
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
from typing import Optional

if os.name == "nt":
    from ctypes import wintypes
    import msvcrt
else:
    import select
    import termios
    import tty

SDSIO_SERVER_VERSION = "0.9.9"

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

# SDS Flags bit positions
SDS_FLAG_MASK_START        = (1 << 31)
SDS_FLAG_MASK_CI_TERMINATE = (1 << 30)
SDS_FLAG_MASK_PLAYBACK_MODE= (1 << 29)
SDS_FLAG_MASK_ALIVE        = (1 << 28)
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
            max_size = 1024 * 1024 * 1024  # default to 1 GB
        self._buf = bytearray(max_size)
        self._max = max_size
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
            first = min(len(data), self._max - self._tail)
            self._buf[self._tail:self._tail+first] = data[:first]
            self._tail = (self._tail + first) % self._max
            second = len(data) - first
            if second:
                self._buf[self._tail:self._tail+second] = data[first:]
                self._tail = (self._tail + second) % self._max
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
            to_read = min(amt, self._count)
            # read in up to two slices
            first = min(to_read, self._max - self._head)
            data = bytes(self._buf[self._head:self._head+first])
            self._head = (self._head + first) % self._max
            second = to_read - first
            if second:
                data += bytes(self._buf[self._head:self._head+second])
                self._head = (self._head + second) % self._max
            self._count -= to_read
            # wake writers
            self._not_full.notify_all()
            return data

    def set_eof(self):
        with self._lock:
            self.eof = True
            # wake any waiting readers
            self._not_empty.notify_all()


# ---------------------------------------------------------------------------- #
#                               Safe print                                     #
# ---------------------------------------------------------------------------- #
class safe_print:
    _SPINNER = '|/-\\'

    def __init__(self, level=logging.INFO, formatter=None):
        if isinstance(formatter, str):
            formatter = logging.Formatter(formatter)
        self._lock = threading.Lock()
        self._logger = logging.getLogger("sdsio_logger")
        self._spinner_idx = 0
        self._spinner_active = False

        # Avoid adding duplicate handlers if logger already exists
        if not self._logger.handlers:
            handler = logging.StreamHandler()
            if formatter is None:
                formatter = logging.Formatter("[%(levelname)s] %(message)s")
            handler.setFormatter(formatter)
            self._logger.addHandler(handler)

        self._logger.setLevel(level)

    def set_level(self, level):
        with self._lock:
            self._logger.setLevel(level)

    def info(self, msg):
        with self._lock:
            self._clear_progress()
            self._logger.info(msg)

    def debug(self, msg):
        with self._lock:
            self._clear_progress()
            self._logger.debug(msg)

    def warning(self, msg):
        with self._lock:
            self._clear_progress()
            self._logger.warning(msg)

    def error(self, msg):
        with self._lock:
            self._clear_progress()
            self._logger.error(msg)

    def exception(self, msg):
        with self._lock:
            self._clear_progress()
            self._logger.exception(msg)

    def progress_spinner(self, mgr):
        with self._lock:
            if mgr.opened_streams:
                frame = self._SPINNER[self._spinner_idx % len(self._SPINNER)]
                sys.stdout.write(f"\r{frame}")
                sys.stdout.flush()
                self._spinner_idx += 1
                self._spinner_active = True

    def _clear_progress(self):
        if self._spinner_active:
            sys.stdout.write("\r \r")
            sys.stdout.flush()
            self._spinner_active = False
            self._spinner_idx = 0


# Global printer object for logging
printer = safe_print(level=logging.INFO, formatter="%(message)s")

# ---------------------------------------------------------------------------- #
#                            Print status bar                                  #
# ---------------------------------------------------------------------------- #
class StatusBar:
    def __init__(self, manager, interval=0.3):
        self.mgr = manager
        self.interval = interval
        self._stop = threading.Event()
        self._idx = 0
        self._thread = threading.Thread(target=self._run, daemon=True)
        self._thread.start()

    def _run(self):
        while not self._stop.is_set():
            if (self.mgr.time_last_rw + self.interval) > time.time():
                printer.progress_spinner(self.mgr)
            time.sleep(self.interval)

    def stop(self):
        self._stop.set()
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
            printer.info(f"Starting monitor server on port {self._port}.")
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
            soc, _ = self._listening_socket.accept()
            soc.setblocking(False)
            with self._lock:
                self._socket = soc
            self._recv_buf.clear()
            printer.info("Monitor client connected.")
            # Send current info immediately on connection
            self.send_info_msg(*self._last_info)
        except socket.timeout:
            pass
        except Exception:
            pass

    def handle_commands(self):
        if self._listening_socket is None:
            return

        if self._socket is None:
            self._accept_client()

        if self._socket is None:
            return

        # Receive available bytes into the buffer
        soc = self._socket
        try:
            chunk = soc.recv(256)
            if chunk:
                self._recv_buf.extend(chunk)
            else:
                # Peer closed connection gracefully
                printer.info("Monitor client disconnected.")
                with self._lock:
                    if self._socket is soc:
                        try:
                            self._socket.close()
                        except Exception:
                            pass
                        self._socket = None
                self._recv_buf.clear()
                return
        except BlockingIOError:
            pass  # No data available right now
        except Exception:
            printer.info("Monitor client disconnected.")
            with self._lock:
                if self._socket is soc:
                    try:
                        self._socket.close()
                    except Exception:
                        pass
                    self._socket = None
            self._recv_buf.clear()
            return

        # Process all complete 16-byte commands in the buffer
        # Currently only SDSIO_MON_FLAGS is expected from the client. It doesn't have any response.
        while len(self._recv_buf) >= 16:
            cmd = int.from_bytes(self._recv_buf[0:4], 'little')
            if cmd == SDSIO_MON_FLAGS:
                set_flags  = int.from_bytes(self._recv_buf[4:8],  'little')
                clear_flags = int.from_bytes(self._recv_buf[8:12], 'little')
                printer.info(f"Monitor command received: SDSIO_MON_FLAGS (set=0x{set_flags:08X}, clear=0x{clear_flags:08X})")
                if self._flags:
                    self._flags.apply(set_flags, clear_flags)
            else:
                printer.warning(f"Unknown monitor command received: {cmd}")
            del self._recv_buf[:16]

    def _send(self, msg: bytearray):
        """Send msg to the monitor client under the lock."""
        with self._lock:
            if self._socket:
                try:
                    self._socket.sendall(msg)
                except Exception:
                    pass

    def send_open_msg(self, filename: str, mode: int):
        filename_bytes = filename.encode('utf-8')
        msg = bytearray()
        msg.extend(SDSIO_MON_OPEN.to_bytes(4, 'little'))
        msg.extend((0).to_bytes(4, 'little'))
        msg.extend(mode.to_bytes(4, 'little'))
        msg.extend((0).to_bytes(4, 'little'))
        msg.extend(len(filename_bytes).to_bytes(4, 'little'))
        msg.extend(filename_bytes)
        self._send(msg)

    def send_close_msg(self, filename: str):
        filename_bytes = filename.encode('utf-8')
        msg = bytearray()
        msg.extend(SDSIO_MON_CLOSE.to_bytes(4, 'little'))
        msg.extend((0).to_bytes(4, 'little'))
        msg.extend((0).to_bytes(4, 'little'))
        msg.extend((0).to_bytes(4, 'little'))
        msg.extend(len(filename_bytes).to_bytes(4, 'little'))
        msg.extend(filename_bytes)
        self._send(msg)

    def send_info_msg(self, flags: int, idle_rate: int, err_data: bytes):
        self._last_info = (flags, idle_rate, err_data)
        msg = bytearray()
        msg.extend(SDSIO_MON_INFO.to_bytes(4, 'little'))
        msg.extend(flags.to_bytes(4, 'little'))
        msg.extend(idle_rate.to_bytes(4, 'little'))
        msg.extend(len(err_data).to_bytes(4, 'little'))
        msg.extend(err_data)
        self._send(msg)

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

    def apply(self, set_mask: int, clear_mask: int):
        with self._lock:
            self._set   |= set_mask
            self._clear |= clear_mask

    def consume_set(self) -> int:
        with self._lock:
            val = self._set | SDS_FLAG_MASK_ALIVE
            if self._auto_playback:
                val |= SDS_FLAG_MASK_PLAYBACK_MODE
            if val & SDS_FLAG_MASK_PLAYBACK_MODE:
                self._playback_mode = True
            self._set = 0
            return val

    def consume_clear(self) -> int:
        with self._lock:
            val = self._clear
            if not self._auto_playback and (val & SDS_FLAG_MASK_PLAYBACK_MODE):
                self._playback_mode = False
            self._clear = 0
            return val

    @property
    def playback_mode(self):
        return self._playback_mode


# ---------------------------------------------------------------------------- #
#                            SDS IO Control Input                              #
# ---------------------------------------------------------------------------- #
class sdsControlInput(threading.Thread):

    def __init__(self, flags: sdsFlags, monitor: Optional[sdsMonitorInterface] = None):
        super().__init__(daemon=True)
        self._flags   = flags
        self._monitor = monitor
        self._quit    = threading.Event()

        # Mapping of key characters to (set_mask, clear_mask, description)
        self._KEY_ACTIONS = {
            'R': (SDS_FLAG_MASK_START,                              SDS_FLAG_MASK_PLAYBACK_MODE, "start recording"),
            'r': (SDS_FLAG_MASK_START,                              SDS_FLAG_MASK_PLAYBACK_MODE, "start recording"),
            'P': (SDS_FLAG_MASK_START | SDS_FLAG_MASK_PLAYBACK_MODE, 0,                         "start playback"),
            'p': (SDS_FLAG_MASK_START | SDS_FLAG_MASK_PLAYBACK_MODE, 0,                         "start playback"),
            'S': (0,                                                SDS_FLAG_MASK_START,         "stop"),
            's': (0,                                                SDS_FLAG_MASK_START,         "stop"),
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

    def run(self):
        while not self._quit.is_set():

            # Poll monitor for incoming commands
            if self._monitor:
                self._monitor.handle_commands()

            # Read from keyboard
            ch = self._read_key(timeout=0.01)
            if ch is None:
                continue
            if ch in ('x', 'X'):
                printer.info(f"sdsControl: terminate ('{ch}')")
                os.kill(os.getpid(), signal.SIGINT)
                return
            action = self._KEY_ACTIONS.get(ch)
            if action:
                set_mask, clear_mask, desc = action
                self._flags.apply(set_mask, clear_mask)
                printer.info(f"sdsControl: {desc} ('{ch}')")

    def stop(self):
        self._quit.set()

    if os.name == "nt":
        def _read_key(self, timeout=0.1):
            end_time = time.time() + timeout
            while time.time() < end_time and not self._quit.is_set():
                if msvcrt.kbhit():
                    ch = msvcrt.getwch()

                    # Ignore special keys like arrows and function keys.
                    if ch in ("\x00", "\xe0"):
                        if msvcrt.kbhit():
                            msvcrt.getwch()
                        return None

                    return ch

                time.sleep(0.01)
            return None
    else:
        def _read_key(self, timeout=0.1):
            import select
            import termios
            import tty

            fd = sys.stdin.fileno()
            old_settings = termios.tcgetattr(fd)
            try:
                tty.setcbreak(fd)
                ready, _, _ = select.select([sys.stdin], [], [], timeout)
                if ready:
                    return sys.stdin.read(1)
                return None
            finally:
                termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)


# ---------------------------------------------------------------------------- #
#                            SDS IO Manager                                    #
# ---------------------------------------------------------------------------- #
class sdsio_manager:
    def __init__(self, work_dir, auto_playback=False, play_list: Optional[list] = None, mon_port: Optional[int] = None):
        self.stream_id = 0
        self.play_step_index = 0
        self.default_work_dir = path.normpath(work_dir)
        self.work_dir = self.default_work_dir
        self.opened_streams = {}    # sid -> (name, mode)
        self.label_list = []
        self.timestamp_list = []
        # write side
        self.write_buffers = {}     # sid -> ByteStreamBuffer
        self.write_threads = {}     # sid -> Thread
        self.write_stop = {}        # sid -> Event
        # read side
        self.read_buffers = {}      # sid -> ByteStreamBuffer
        self.read_threads = {}      # sid -> Thread
        self.read_stop = {}         # sid -> Event
        # lock to protect stream_id increment and open checks
        self.manager_lock = threading.Lock()
        # timestamp of last stream read or write command
        self.time_last_rw = time.time()
        # status bar
        self.status = StatusBar(self)

        self.playback_mode = False
        self.play_list = play_list
        self.mon_port = mon_port
        # SDS Control Flags
        printer.info("Starting SDS Control Flags thread. R=record, P=playback, S/s=stop, X/x=terminate, A-H=set flags 0-7, a-h=clear flags 0-7.")
        self.flags      = sdsFlags(auto_playback)
        self.monitor    = sdsMonitorInterface(self.mon_port, self.flags) if self.mon_port else None
        self.ctrl_input = sdsControlInput(self.flags, self.monitor)

        self.info_flags: int = 0
        self.info_IdleRate: int = 0
        self._last_async_time = time.time()

    def _shutdown(self):
        if self.ctrl_input:
            self.ctrl_input.stop()
            self.ctrl_input.join(timeout=2.0)
            self.ctrl_input = None
        if self.monitor:
            self.monitor.close()
            self.monitor = None
        self.clean()

    def _build_timestamp_boundaries(self, name: str) -> list[int]:
        timestampes = []
        for label in self.label_list:
            # Open playback sds file and read timestamop of first packet
            sds_file_path = path.join(self.work_dir, f"{name}.{label}.sds")
            with open(sds_file_path, "rb") as f:
                header = f.read(8)
                if len(header) == 8:
                    ts = int.from_bytes(header[0:4], 'little')
                    timestampes.append(ts)
                else:
                    # Error: cant get timestamp
                    timestampes.append(0xFFFFFFFF)
        return timestampes

    def _file_write_worker(self, sid, name, buf: ByteStreamBuffer, stop_evt):
        data = bytearray()
        try:
            if self.playback_mode:
                # In playback mode, wait until label_list and timestamp_list are populated
                # timestamp_list is generted when playback file is opened
                while not self.label_list or not self.timestamp_list:
                    if stop_evt.is_set():
                        return
                    time.sleep(0.1)

            for index in range(len(self.label_list)):
                if stop_evt.is_set():
                    break

                label = self.label_list[index]
                sds_file_name = f"{name}.{label}.p.sds" if self.playback_mode else f"{name}.{label}.sds"
                sds_file_path = path.join(self.work_dir, sds_file_name)

                # Check if sds file exsist. If so, rename it to *.bak
                if path.exists(sds_file_path):
                    if path.exists(sds_file_path + ".bak"):
                        try:
                            os.remove(sds_file_path + ".bak")
                        except Exception:
                            printer.warning(f"Could not delete backup file '{sds_file_path}.bak'")
                    try:
                        os.rename(sds_file_path, sds_file_path + ".bak")
                    except Exception:
                        printer.warning(f"Could not create backup for existing file '{sds_file_path}'")

                eof_reached = False
                with open(sds_file_path, "wb") as file_obj:
                    stream_name, _ = self.opened_streams[sid]
                    printer.info(f"Record:   {stream_name} ({sds_file_path})")
                    if self.monitor:
                        self.monitor.send_open_msg(sds_file_path, 1)
                    while True:
                        data_sz = len(data)

                        if data_sz < 8:
                            # Accumulate header bytes
                            chunk = buf.read(8 - data_sz, timeout=0.1)
                            if chunk:
                                data += chunk
                            elif buf.eof:
                                # Incomplete header at EOF — discard fragment and stop
                                data = bytearray()
                                eof_reached = True
                                break
                            elif stop_evt.is_set():
                                # Read timed out and stream was closed — no more data expected
                                data = bytearray()
                                eof_reached = True
                                break
                            # else: timeout, try again

                        else:
                            # Full 8-byte header available
                            timestamp = int.from_bytes(data[0:4], 'little')
                            data_block_size = int.from_bytes(data[4:8], 'little')

                            # Check if this record marks the boundary of the next label
                            if self.timestamp_list and index + 1 < len(self.timestamp_list):
                                if timestamp == self.timestamp_list[index + 1]:
                                    break  # keep data — it will be written to the next label file

                            needed = 8 + data_block_size
                            if data_sz < needed:
                                # Accumulate payload bytes
                                chunk = buf.read(needed - data_sz, timeout=0.1)
                                if chunk:
                                    data += chunk
                                elif buf.eof:
                                    # Incomplete record at EOF — discard and stop
                                    data = bytearray()
                                    eof_reached = True
                                    break
                                elif stop_evt.is_set():
                                    # Read timed out and stream was closed — no more data expected
                                    data = bytearray()
                                    eof_reached = True
                                    break
                                # else: timeout, try again
                            else:
                                # Complete record: write and reset for next record
                                file_obj.write(data)
                                data = bytearray()
                printer.info(f"Closed:   {name} ({sds_file_path})")
                if self.monitor:
                    self.monitor.send_close_msg(sds_file_path)
                if eof_reached:
                    break
        except Exception:
            printer.exception(f"Writer {sid} error")

    def _file_read_worker(self, sid, name, buf: ByteStreamBuffer, stop_evt):
        chunk_size = 128 * 1024
        try:
            for label in self.label_list:
                if stop_evt.is_set():
                    break
                sds_file_path = path.join(self.work_dir, f"{name}.{label}.sds")
                with open(sds_file_path, "rb") as file_obj:
                    printer.info(f"Playback: {name} ({sds_file_path})")
                    if self.monitor:
                        self.monitor.send_open_msg(sds_file_path, 0)
                    while not stop_evt.is_set():
                        data = file_obj.read(chunk_size)
                        if data:
                            buf.write(data)
                        else:
                            break  # EOF on this file, move to next label
                printer.info(f"Closed:   {name} ({sds_file_path})")
                if self.monitor:
                    self.monitor.send_close_msg(sds_file_path)
        except Exception:
            printer.exception(f"Reader {sid} error")
        finally:
            buf.set_eof()

    def _create_play_label_list(self, name) -> list[str]:
        labels = []
        if self.play_list and self.play_step_index < len(self.play_list):
            step = self.play_list[self.play_step_index]
            labels = list(step.get('labels', []))
        else:
            # No playlist: one file per open, indexed by play_step_index
            candidate = path.join(self.work_dir, f"{name}.{self.play_step_index}.sds")
            if path.exists(candidate):
                labels.append(str(self.play_step_index))
        return labels

    def _open(self, mode, name):
        cmd = CMD_OPEN
        # prepare error response
        resp_err = bytearray()
        resp_err.extend(cmd.to_bytes(4,'little'))
        resp_err.extend((0).to_bytes(4,'little'))
        resp_err.extend(mode.to_bytes(4,'little'))
        resp_err.extend((0).to_bytes(4,'little'))

        # validate name
        if len(name) == 0:
            printer.info(f"Invalid stream name: {name}")
            return resp_err
        invalid_chars = [chr(i) for i in range(0x00, 0x10)] + [chr(0x7F), '"', '*', '/', ':', '<', '>', '?', '\\', '|']
        if any(ch in name for ch in invalid_chars):
            printer.info(f"Invalid stream name: {name}")
            return resp_err

        # ensure not already open
        if any(n == name for (n, _) in self.opened_streams.values()):
            printer.info(f"Stream '{name}' is already opened, cannot open again.")
            return resp_err

        # update playback mode from flags only when new session is started
        if not self.opened_streams:
            self.playback_mode = self.flags.playback_mode

        # check if new rec/play session
        if self.playback_mode:
            if not self.label_list:
                # Get flags, Set working dir
                if self.play_list:
                    if self.play_step_index < len(self.play_list):
                        step = self.play_list[self.play_step_index]
                        set_flags = step.get('setflags', 0)
                        clear_flags = step.get('clearflags', 0)
                        recdir = step.get('recdir', None)
                        self.work_dir = path.normpath(path.join(self.default_work_dir, recdir)) if recdir else self.default_work_dir
                    else:
                        printer.error(f"Open Failed. End of playlist. No more steps available for playback stream '{name}'.")
                        return resp_err
                else:
                    self.work_dir = self.default_work_dir
                    set_flags = 0
                    clear_flags = 0

                if set_flags or clear_flags:
                    printer.debug(f"Applying flags for playback stream '{name}': set=0x{set_flags:08X}, clear=0x{clear_flags:08X}")
                    self.flags.apply(set_flags, clear_flags)

                # Create label list
                play_label_list = self._create_play_label_list(name)
                if not play_label_list:
                    if not self.play_list and self.play_step_index > 0:
                        printer.error(f"Open Failed. No more files available for playback stream '{name}'.")
                    else:
                        printer.error(f"Open Failed. No files found for playback stream '{name}'.")
                    return resp_err
                self.label_list = play_label_list

        else:
            if mode == 0:
                printer.info(f"Cannot open stream '{name}' for playback. Playback mode is not enabled.")
                return resp_err
            self.work_dir = self.default_work_dir

        # mode 1 = write, 0 = read
        if mode == 1:
            if not self.label_list:
                if not self.playback_mode:
                    # find first available numeric label for new recording
                    idx = 0
                    sds_file_path = path.join(self.work_dir, f"{name}.{idx}.sds")
                    while path.exists(sds_file_path):
                        idx += 1
                        sds_file_path = path.join(self.work_dir, f"{name}.{idx}.sds")
                    self.label_list.append(str(idx))

            # allocate new sid
            with self.manager_lock:
                self.stream_id += 1
                sid = self.stream_id
            self.opened_streams[sid] = (name, mode)
            buf = ByteStreamBuffer()
            stop_evt = threading.Event()
            thr = threading.Thread(
                target=self._file_write_worker,
                args=(sid, name, buf, stop_evt),
                daemon=True
            )
            thr.start()
            self.write_buffers[sid] = buf
            self.write_threads[sid] = thr
            self.write_stop[sid]   = stop_evt

        else:
            # Validate that files for all labels exist
            for label in self.label_list:
                sds_file_path = path.join(self.work_dir, f"{name}.{label}.sds")
                if not path.exists(sds_file_path):
                    printer.error(f"Missing file for playback stream '{name}': {sds_file_path}")
                    return resp_err

            if not self.timestamp_list:
                self.timestamp_list = self._build_timestamp_boundaries(name)

            # allocate new sid
            with self.manager_lock:
                self.stream_id += 1
                sid = self.stream_id

            self.opened_streams[sid] = (name, mode)

            buf = ByteStreamBuffer()
            stop_evt = threading.Event()
            thr = threading.Thread(
                target=self._file_read_worker,
                args=(sid, name, buf, stop_evt),
                daemon=True
            )
            thr.start()
            self.read_buffers[sid] = buf
            self.read_threads[sid] = thr
            self.read_stop[sid]  = stop_evt

        # build success response
        resp = bytearray()
        resp.extend(cmd.to_bytes(4,'little'))
        resp.extend(sid.to_bytes(4,'little'))
        resp.extend(mode.to_bytes(4,'little'))
        resp.extend((0).to_bytes(4,'little'))

        return resp

    def _close(self, sid):
        resp = bytearray()
        name = self.opened_streams[sid][0]

        # clean up writer side
        if sid in self.write_buffers:
            buf = self.write_buffers.pop(sid)
            buf.set_eof()
            self.write_stop[sid].set()
            self.write_threads[sid].join()
            self.write_threads.pop(sid)
            self.write_stop.pop(sid)
        # clean up reader side
        if sid in self.read_buffers:
            self.read_stop[sid].set()
            self.read_threads[sid].join()
            self.read_buffers.pop(sid)
            self.read_threads.pop(sid)
            self.read_stop.pop(sid)
        # unregister stream
        self.opened_streams.pop(sid, None)

        if not self.opened_streams:
            if self.playback_mode:
                self.play_step_index += 1
            self.label_list.clear()
            self.timestamp_list.clear()

        return resp

    def _write(self, sid, data):
        resp = bytearray()
        buf = self.write_buffers.get(sid)
        if not buf:
            printer.info(f"Not opened for write: {sid}")
            return resp
        buf.write(data)

        self.time_last_rw = time.time()
        return resp

    def _read(self, sid, size):
        resp = bytearray()
        cmd = CMD_READ
        eof = 0
        data = bytearray()
        entry = self.opened_streams.get(sid)
        # invalid read
        if not entry or entry[1] != 0:
            resp.extend(cmd.to_bytes(4,'little'))
            resp.extend(sid.to_bytes(4,'little'))
            resp.extend((0).to_bytes(4,'little'))
            resp.extend((0).to_bytes(4,'little'))
            return resp

        buf = self.read_buffers.get(sid)
        # read until requested size or EOF
        while len(data) < size:
            chunk = buf.read(size - len(data), timeout=0.05)
            if not chunk:
                break
            data.extend(chunk)
        if not data and buf.eof:
            eof = 1
        resp.extend(cmd.to_bytes(4,'little'))
        resp.extend(sid.to_bytes(4,'little'))
        resp.extend(eof.to_bytes(4,'little'))
        resp.extend(len(data).to_bytes(4,'little'))
        if data:
            resp.extend(data)

        self.time_last_rw = time.time()
        return resp

    def _pingServer(self, sid):
        resp = bytearray()
        cmd = CMD_PING
        resp.extend(cmd.to_bytes(4,'little'))
        resp.extend(sid.to_bytes(4,'little'))
        resp.extend((1).to_bytes(4,'little'))
        resp.extend((0).to_bytes(4,'little'))
        printer.info("Ping received.")
        return resp

    def _info(self, flags: int, idle_rate: int, err_data: bytes):
        # Print info: sdsFlags, sdsIdleRate, Error
        resp = bytearray()
        if self.info_flags != flags:
            printer.info(f"sdsFlags = 0x{flags:08X}")
            self.info_flags = flags
        if idle_rate and self.info_IdleRate != idle_rate:
            if idle_rate != 0xFFFFFFFF:
                printer.info(f"{idle_rate}% idle")
            self.info_IdleRate = idle_rate
        if err_data:
            status = int.from_bytes(err_data[0:4],'little')
            line   = int.from_bytes(err_data[4:8],'little')
            err_mgs = err_data[8:]
            printer.info(f"sdsInfoError: status=0x{status:08X}, line={line}, msg={err_mgs.decode('utf-8', errors='replace')}")

        if self.monitor:
            self.monitor.send_info_msg(flags, idle_rate, err_data)

        return resp

    def clean(self):
        # close all open streams
        for sid in list(self.opened_streams.keys()):
            self._close(sid)

    def get_async_flags(self):
        resp = bytearray()
        cmd = CMD_FLAGS
        set_mask   = self.flags.consume_set()
        clear_mask = self.flags.consume_clear()
        resp.extend(cmd.to_bytes(4,'little'))
        resp.extend(set_mask.to_bytes(4,'little'))
        resp.extend(clear_mask.to_bytes(4,'little'))
        resp.extend((0).to_bytes(4,'little'))
        return resp

    def get_async_response(self):
        now = time.time()
        if now - self._last_async_time >= 0.1:
            self._last_async_time = now
            return self.get_async_flags()
        return None

    def get_shutdown_flags(self):
        resp = bytearray()
        cmd = CMD_FLAGS
        resp.extend(cmd.to_bytes(4,'little'))
        resp.extend((0).to_bytes(4,'little'))
        resp.extend((1 << 28).to_bytes(4,'little'))
        resp.extend((0).to_bytes(4,'little'))
        return resp

    def execute_request(self, buf: bytes):
        cmd = int.from_bytes(buf[0:4],'little')
        if cmd in CMD_SYNC:
            sid = int.from_bytes(buf[4:8],'little')
            arg = int.from_bytes(buf[8:12],'little')
            sz  = int.from_bytes(buf[12:16],'little')
            data= buf[16:16+sz]
            if   cmd == CMD_OPEN:  return self._open(arg, data.decode('utf-8').rstrip('\0'))
            elif cmd == CMD_CLOSE: return self._close(sid)
            elif cmd == CMD_WRITE: return self._write(sid, data)
            elif cmd == CMD_READ:  return self._read(sid, arg)
            elif cmd == CMD_PING:  return self._pingServer(sid)
        elif cmd == CMD_INFO:
            flags     = int.from_bytes(buf[4:8],'little')
            idle_rate = int.from_bytes(buf[8:12],'little')
            err_len   = int.from_bytes(buf[12:16],'little')
            err_data= buf[16:16+err_len]
            return self._info(flags, idle_rate, err_data)

        else:
            printer.error(f"=== FATAL ERROR === : Data integrity error - protocol mismatch. Restart the SDSIO Client.")
            return bytearray()


# ---------------------------------------------------------------------------- #
#                            Async Socket Server                               #
# ---------------------------------------------------------------------------- #
class async_sdsio_server_socket:
    def __init__(self, ip, port, manager: sdsio_manager):
        self.ip = ip
        self.port = port
        self.manager = manager
        self.server = None
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

    async def handle_client(self, reader: asyncio.StreamReader, writer: asyncio.StreamWriter):
        task = asyncio.current_task()
        self._handler_tasks.add(task)
        try:
            printer.info(f"SDSIO Client connected.")
            while True:
                # Send async FLAGS response periodically
                resp = self.manager.get_async_response()
                if resp:
                    writer.write(resp)
                    await writer.drain()

                try:
                    # read fixed-size header, then payload, with a timeout
                    hdr = await asyncio.wait_for(reader.readexactly(16), timeout=0.1)
                except asyncio.TimeoutError:
                    continue # No data from client, loop to check for FLAGS send

                # validate command before reading payload
                cmd = int.from_bytes(hdr[0:4],'little')
                if cmd not in CMD_ALL:
                    printer.error(f"=== FATAL ERROR === : Data integrity error - protocol mismatch. Restart the SDSIO Client.")
                    printer.info("Closing SDSIO Client connection...")
                    break

                sz  = int.from_bytes(hdr[12:16],'little')
                pl  = await reader.readexactly(sz) if sz > 0 else b''
                resp= self.manager.execute_request(hdr + pl)
                if resp:
                    writer.write(resp)
                    await writer.drain()
        except asyncio.CancelledError:
            raise                          # re-raise per docs
        except (asyncio.IncompleteReadError, ConnectionResetError, OSError):
            if not self._shutting_down:
                printer.info(f"SDSIO Client disconnected.")
        finally:
            self._handler_tasks.discard(task)
            # Send shutdown flags (clear alive bit) before closing
            try:
                writer.write(self.manager.get_shutdown_flags())
                await writer.drain()
            except Exception:
                pass
            await self._safe_close(reader, writer)
            # Clean up any streams
            self.manager.clean()
            if not self._shutting_down:
                printer.info("Waiting for SDSIO Client to reconnect...")

    async def start(self):
        self.server = await asyncio.start_server(self.handle_client, self.ip, self.port)
        addr = self.server.sockets[0].getsockname()
        printer.info(f"Socket server listening on {addr[0]}:{addr[1]}")
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
            for task in list(self._handler_tasks):
                task.cancel()
            if self._handler_tasks:
                done, pending = await asyncio.wait(
                    list(self._handler_tasks), timeout=2.0
                )
                for task in pending:
                    task.cancel()

async def sdsio_server_socket_run_supervised(ip, port, manager):
    while True:
        srv = async_sdsio_server_socket(ip, port, manager)
        try:
            await srv.start()
            # If start() returns normally, break out
            printer.info("Server shut down cleanly.")
            break
        except Exception:
            printer.info(f"Server fatal error.")
            printer.info("Server restarting...")
            # Clean up any streams
            manager.clean()
            # Close the listener socket if it’s open
            if srv.server:
                srv.server.close()
                await srv.server.wait_closed()
            await asyncio.sleep(1)


# ---------------------------------------------------------------------------- #
#                           Blocking Serial Server                             #
# ---------------------------------------------------------------------------- #
class sdsio_server_serial:
    def __init__(self, port, baudrate, parity, stop_bits, connect_timeout, manager: sdsio_manager):
        self.port = port
        self.baudrate = baudrate
        self.parity = parity
        self.stop_bits = stop_bits
        self.connect_timeout = connect_timeout
        self.manager = manager
        self.ser = None

    def open(self):
        first_attempt = True
        printer.info(f"Serial Port: {self.port}")
        try:
            self.ser = serial.Serial()
            if sys.platform != "darwin":
                if 'COM' in self.port:
                    self.ser.port = self.port
                else:
                    self.ser.port = f"COM{self.port}"
            else:
                self.ser.port = f"/dev/tty{self.port}"
            self.ser.baudrate = self.baudrate
            self.ser.parity = self.parity
            self.ser.stopbits = self.stop_bits
            self.ser.timeout = 0
        except Exception:
            printer.info("Error initializing serial.")
            sys.exit(1)
        start_time = time.time()
        while True:
            try:
                self.ser.open()
                printer.info("Serial port opened successfully.")
                break
            except Exception:
                if first_attempt:
                    printer.info(f"Waiting for Client on {self.port}...")
                    first_attempt = False
                if self.connect_timeout and (time.time() - start_time >= self.connect_timeout):
                    printer.info(f"Serial port open failed after {self.connect_timeout} seconds.")
                    sys.exit(1)
                time.sleep(0.5)

    def close(self):
        try:
            self.ser.close()
        except Exception:
            pass

    def read(self, size):
        try:
            return self.ser.read(size)
        except Exception:
            printer.info("Serial read error")
            raise

    def write(self, data):
        try:
            return self.ser.write(data)
        except Exception:
            printer.info("Serial write error")
            raise

    def start(self):
        self.open()
        printer.info("Serial Server started.")
        buffer = bytearray()

        try:
            while True:
                # Send async FLAGS response periodically
                resp = self.manager.get_async_response()
                if resp:
                    self.write(resp)

                data = self.read(16 * 1024)
                if data:
                    buffer.extend(data)
                else:
                    time.sleep(0.001)
                    continue

                # Process complete messages from the buffer...
                while len(buffer) >= 16:
                    header = buffer[:16]

                    # validate command before reading payload
                    cmd = int.from_bytes(header[0:4], 'little')
                    if cmd not in CMD_ALL:
                        printer.error(f"=== FATAL ERROR === : Data integrity error - protocol mismatch. Restart the SDSIO Client.")
                        buffer.clear()
                        return  # Exit start(), finally block will clean up

                    data_size = int.from_bytes(header[12:16], 'little')
                    req_len = 16 + data_size
                    if len(buffer) < req_len:
                        break
                    request_buf = buffer[:req_len]
                    buffer = buffer[req_len:]
                    response = self.manager.execute_request(request_buf)
                    if response:
                        self.write(response)
        finally:
            # Send shutdown flags (clear alive bit) before closing
            try:
                self.write(self.manager.get_shutdown_flags())
            except Exception:
                pass
            # Clean up all SDS streams on disconnect/error
            self.manager.clean()
            self.ser.close()

def sdsio_server_serial_run_supervised(port, baudrate, parity, stop_bits, connect_timeout, manager):
    while True:
        try:
            srv = sdsio_server_serial(
                port, baudrate, parity,
                stop_bits, connect_timeout, manager
            )
            srv.start()
            # start() returned normally (e.g., invalid command)
            printer.info("Server restarting...")
        except Exception:
            printer.info(f"Server fatal error.")
            printer.info("Server restarting...")
            # Reset all open streams
            manager.clean()
            # Try to close the port if it's still open
            try:
                srv.close()
            except Exception:
                pass
            time.sleep(1)
        time.sleep(0.1)


# ---------------------------------------------------------------------------- #
#                       Async USB-Bulk Server (usb1)                           #
# ---------------------------------------------------------------------------- #
class sdsio_server_usb:
    PRODUCT_STR = "SDSIO Client"
    XFER_NUM    = 32
    XFER_SIZE   = 8 * 1024  # 8 KiB per transfer

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
        PROCESS_REALTIME              = 0x00000100
        THREAD_PRIORITY_TIME_CRITICAL = 15

    def __init__(self, manager: 'sdsio_manager', loop: asyncio.AbstractEventLoop, high_priority: bool = False):
        self.mgr             = manager
        self.loop            = loop
        self.high_priority   = high_priority
        self.ctx             = None
        self.handle          = None
        self.in_ep           = None
        self.out_ep          = None
        self.in_transfers    = []
        self.out_pool        = []
        self.out_in_flight   = set()
        self.in_q            = asyncio.Queue()
        self.out_q           = asyncio.Queue()
        self.running         = False
        self._rx_buf         = bytearray()
        self._poll_thread    = None
        self._monitor_thread = None
        self.vendor_id       = None
        self.product_id      = None
        self._protocol_error = False
        self._shutdown_event = threading.Event()
        self._disconnect_lock = threading.Lock()

    async def open(self):
        first_attempt = True
        try:
            usb_dev = None
            while usb_dev is None:
                for dev in self.ctx.getDeviceList(skip_on_error=True):
                    try:
                        if dev.getProduct() == self.PRODUCT_STR:
                            usb_dev = dev
                            break
                    except usb1.USBError:
                        continue
                if usb_dev is None:
                    if first_attempt:
                        printer.info("Waiting for SDSIO Client USB device...")
                        first_attempt = False
                    await asyncio.sleep(0.5)

            # remember IDs for reconnect detection
            self.vendor_id  = usb_dev.getVendorID()
            self.product_id = usb_dev.getProductID()

            self.handle = usb_dev.open()
            try:
                if self.handle.kernelDriverActive(0):
                    self.handle.detachKernelDriver(0)
            except Exception:
                pass
            self.handle.claimInterface(0)

            # discover bulk endpoints
            for cfg in usb_dev.iterConfigurations():
                for intf in cfg.iterInterfaces():
                    for sett in intf.iterSettings():
                        for ep in sett.iterEndpoints():
                            if (ep.getAttributes() & usb1.TRANSFER_TYPE_MASK) != usb1.TRANSFER_TYPE_BULK:
                                continue
                            addr = ep.getAddress()
                            if addr & usb1.ENDPOINT_DIR_MASK:
                                self.in_ep       = addr
                                self.in_pckt_sz  = ep.getMaxPacketSize()
                            else:
                                self.out_ep      = addr
                                self.out_pckt_sz = ep.getMaxPacketSize()

            if not (self.in_ep and self.out_ep):
                raise RuntimeError("Bulk endpoints not found.")

        except asyncio.CancelledError:
            raise

    def _tune_current_thread(self):
        """Best-effort real-time tuning."""
        try:
            if os.name == "nt":
                hProc = self._KERNEL32.GetCurrentProcess()
                self._KERNEL32.SetPriorityClass(hProc, self.PROCESS_REALTIME)
                hThr = self._KERNEL32.GetCurrentThread()
                self._KERNEL32.SetThreadPriority(hThr, self.THREAD_PRIORITY_TIME_CRITICAL)
                self._KERNEL32.SetThreadAffinityMask(hThr, 1)
            else:
                libc_name = "libc.so.6" if sys.platform.startswith("linux") else "libc.dylib"
                libc = ctypes.CDLL(libc_name, use_errno=True)
                SCHED_FIFO = 1
                class sched_param(ctypes.Structure):
                    _fields_ = [("sched_priority", ctypes.c_int)]
                max_prio = libc.sched_get_priority_max(SCHED_FIFO)
                param    = sched_param(max_prio)
                tid      = libc.pthread_self()
                libc.pthread_setschedparam(tid, SCHED_FIFO, ctypes.byref(param))
                try:
                    os.nice(-20)
                except OSError:
                    pass
        except Exception:
            printer.exception("[TUNE] thread tuning failed.")

    def _libusb_loop(self):
        if self.high_priority:
            self._tune_current_thread()
        while self.running:
            try:
                self.ctx.handleEventsTimeout(tv=0)
            except usb1.USBErrorInterrupted:
                pass

    def _signal_disconnect(self):
        """Thread-safe: stop session and wake coroutines.

        Does NOT print any message — the caller in start() determines
        whether the device is truly gone after gather() exits.
        """
        with self._disconnect_lock:
            if not self.running:
                return                          # already handled
            self.running = False
        try:
            self.loop.call_soon_threadsafe(self.in_q.put_nowait,  b'')
            self.loop.call_soon_threadsafe(self.out_q.put_nowait, b'')
        except Exception:
            pass

    def _on_in_complete(self, xfer: usb1.USBTransfer):
        status = xfer.getStatus()
        if status == usb1.TRANSFER_COMPLETED:
            data = bytes(xfer.getBuffer()[:xfer.getActualLength()])
            self.loop.call_soon_threadsafe(self.in_q.put_nowait, data)
        if self.running:
            try:
                xfer.submit()
            except usb1.USBError as e:
                # Track dead transfers; if all IN transfers fail, trigger reconnect
                self._dead_in_xfers.add(id(xfer))
                if len(self._dead_in_xfers) >= len(self.in_transfers):
                    self._signal_disconnect()

    def _on_out_complete(self, xfer: usb1.USBTransfer):
        self.out_in_flight.discard(xfer)
        self.out_pool.append(xfer)

    def _on_hotplug(self, context, device, event):
        if event & usb1.HOTPLUG_EVENT_DEVICE_LEFT:
            self._signal_disconnect()

    def _monitor_loop(self):
        miss_count = 0
        MISS_THRESHOLD = 3          # require 3 consecutive misses (~1.5 s)
        poll_ctx = usb1.USBContext()
        try:
            while self.running:
                time.sleep(0.5)
                found = False
                for dev in poll_ctx.getDeviceList(skip_on_error=True):
                    try:
                        if (dev.getVendorID()  == self.vendor_id and
                            dev.getProductID() == self.product_id):
                            found = True
                            break
                    except usb1.USBError:
                        continue
                if found:
                    miss_count = 0
                else:
                    miss_count += 1
                    if miss_count >= MISS_THRESHOLD:
                        self._signal_disconnect()
                        break
        finally:
            try:
                poll_ctx.close()
            except Exception:
                pass

    async def _consumer(self):
        while self.running:
            # Send async FLAGS response periodically
            resp = self.mgr.get_async_response()
            if resp:
                await self.out_q.put(resp)

            try:
                data = await asyncio.wait_for(self.in_q.get(), timeout=0.1)
            except asyncio.TimeoutError:
                continue  # No data from device, loop to check for FLAGS send

            self._rx_buf.extend(data)
            while len(self._rx_buf) >= 16:
                hdr   = self._rx_buf[:16]

                # validate command before reading payload
                cmd = int.from_bytes(hdr[0:4],'little')
                if cmd not in CMD_ALL:
                    printer.error(f"=== FATAL ERROR === : Data integrity error - protocol mismatch. Restart the SDSIO Client.")
                    self._protocol_error = True
                    self.running = False
                    self._rx_buf.clear()
                    # Wake up _out_sender so it can exit cleanly
                    self.out_q.put_nowait(b'')
                    break

                sz    = int.from_bytes(hdr[12:16], 'little')
                total = 16 + sz
                if len(self._rx_buf) < total:
                    break
                payload = bytes(self._rx_buf[16:total])
                del self._rx_buf[:total]
                resp = self.mgr.execute_request(hdr + payload)
                if resp:
                    await self.out_q.put(resp)
            self.in_q.task_done()

    async def _out_sender(self):
        while self.running or not self.out_q.empty():
            resp = await self.out_q.get()
            # wait for an available OUT-URB
            while self.running and not self.out_pool:
                await asyncio.sleep(0)
            if not self.out_pool:
                break
            xfer = self.out_pool.pop()
            try:
                xfer.setBulk(self.out_ep, resp,
                             callback=self._on_out_complete, timeout=0)
                xfer.submit()
                self.out_in_flight.add(xfer)
            except usb1.USBError:
                break
            finally:
                self.out_q.task_done()

    async def start(self):
        _silent_reconnect = False
        while True:
            self.ctx = usb1.USBContext()
            self.handle          = None
            self.in_ep           = None
            self.out_ep          = None
            self.in_transfers    = []
            self.out_pool        = []
            self.out_in_flight   = set()
            self.in_q            = asyncio.Queue()
            self.out_q           = asyncio.Queue()
            self.running         = False
            self._rx_buf         = bytearray()
            self._poll_thread    = None
            self._monitor_thread = None
            self.vendor_id       = None
            self.product_id      = None
            self._protocol_error = False
            self._dead_in_xfers  = set()
            self._shutdown_event.clear()

            try:
                await self.open()
            except asyncio.CancelledError:
                # Ctrl+C while waiting for device — close USB context to avoid leak
                try:
                    self.ctx.close()
                except Exception:
                    pass
                raise

            # hotplug if possible...
            try:
                self.ctx.hotplugRegisterCallback(
                    callback   = self._on_hotplug,
                    events     = usb1.HOTPLUG_EVENT_DEVICE_LEFT,
                    flags      = 0,
                    vendor_id  = 0,
                    product_id = 0,
                    dev_class  = usb1.HOTPLUG_MATCH_ANY
                )
            except (AttributeError, usb1.USBError):
                # Hotplug not available; use polling monitor thread
                self.running = True
                self._monitor_thread = threading.Thread(
                    target=self._monitor_loop,
                    name="usb-monitor",
                    daemon=True
                )
                self._monitor_thread.start()

            # prepare OUT‐URB pool
            for _ in range(self.XFER_NUM):
                x = self.handle.getTransfer()
                x.setBulk(self.out_ep, bytearray(self.XFER_SIZE),
                          callback=self._on_out_complete, timeout=0)
                self.out_pool.append(x)

            # fire IN‐URBs
            for _ in range(self.XFER_NUM):
                x = self.handle.getTransfer()
                x.setBulk(self.in_ep, bytearray(self.XFER_SIZE),
                          callback=self._on_in_complete, timeout=0)
                if hasattr(x, "setShortNotOk"):
                    x.setShortNotOk(True)
                x.submit()
                self.in_transfers.append(x)

            # Mark server active in the normal path too (hotplug working)
            self.running = True
            if not _silent_reconnect:
                printer.info(f"SDSIO Client USB device connected.")
            else:
                printer.debug("USB session re-established.")
                _silent_reconnect = False

            # start polling thread
            self._poll_thread = threading.Thread(
                target=self._libusb_loop, name="usb-poll", daemon=True
            )
            self._poll_thread.start()

            # run until disconnect or until close() flips self.running=False
            try:
                await asyncio.gather(self._consumer(), self._out_sender())
            except asyncio.CancelledError:
                # Send shutdown flags (clear alive bit) before cleanup
                try:
                    self.handle.bulkWrite(self.out_ep, self.mgr.get_shutdown_flags(), timeout=1000)
                except Exception:
                    pass
                self.mgr.clean()
                self.close()
                raise

            # Send shutdown flags (clear alive bit) before cleanup
            try:
                self.handle.bulkWrite(self.out_ep, self.mgr.get_shutdown_flags(), timeout=1000)
            except Exception:
                pass

            # clean up this connection
            protocol_err = self._protocol_error
            self.mgr.clean()
            self.close()

            # Determine whether the device is truly gone (using a fresh context
            # so we don't rely on the now-closed self.ctx).
            if not protocol_err:
                check_ctx = usb1.USBContext()
                try:
                    device_present = any(
                        dev for dev in check_ctx.getDeviceList(skip_on_error=True)
                        if (dev.getVendorID()  == self.vendor_id and
                            dev.getProductID() == self.product_id)
                    )
                except Exception:
                    device_present = False
                finally:
                    try:
                        check_ctx.close()
                    except Exception:
                        pass

                if device_present:
                    # Transient USB failure — device is still plugged in.
                    # Loop back and silently reconnect.
                    printer.debug("USB transfers interrupted; reconnecting...")
                    _silent_reconnect = True
                    continue
                else:
                    printer.info("USB device disconnected.")

            # On protocol error, wait for device to be physically reset
            if protocol_err:
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
        vid = self.vendor_id
        pid = self.product_id
        if not vid or not pid:
            return

        poll_ctx = usb1.USBContext()
        try:
            def _device_present():
                for dev in poll_ctx.getDeviceList(skip_on_error=True):
                    try:
                        if dev.getVendorID() == vid and dev.getProductID() == pid:
                            return True
                    except usb1.USBError:
                        continue
                return False

            # Wait for device to disappear
            printer.info("Waiting for SDSIO Client USB device to disconnect...")
            while _device_present():
                if self._shutdown_event.wait(0.5):
                    return

            # Wait for device to reappear
            printer.info("Waiting for SDSIO Client USB device to reconnect...")
            while not _device_present():
                if self._shutdown_event.wait(0.5):
                    return
        finally:
            try:
                poll_ctx.close()
            except Exception:
                pass

    def close(self):
        # Called either by KeyboardInterrupt or by internal errors/disconnects
        self.running = False
        self._shutdown_event.set()

        # Ensure any asyncio coroutines blocked on the queues are woken
        try:
            self.loop.call_soon_threadsafe(self.in_q.put_nowait, b'')
            self.loop.call_soon_threadsafe(self.out_q.put_nowait, b'')
        except Exception:
            pass

        for x in self.in_transfers:
            try:
                x.cancel()
            except:
                pass
        self.in_transfers.clear()

        for x in list(self.out_in_flight):
            try:
                x.cancel()
            except:
                pass
        self.out_in_flight.clear()

        for x in self.out_pool:
            try:
                x.cancel()
            except:
                pass
        self.out_pool.clear()

        try:
            self.ctx.handleEventsTimeout(tv=0)
        except:
            pass

        try:
            self.handle.releaseInterface(0)
        except:
            pass
        try:
            self.handle.close()
        except:
            pass
        try:
            self.ctx.close()
        except:
            pass

        if self._poll_thread:
            self._poll_thread.join(timeout=2.0)
        if self._monitor_thread:
            self._monitor_thread.join(timeout=2.0)

async def sdsio_server_usb_run_supervised(manager):
    loop = asyncio.get_running_loop()
    srv  = sdsio_server_usb(manager, loop)
    await srv.start()


# ---------------------------------------------------------------------------- #
#                        Argument Parsing & Entry Point                        #
# ---------------------------------------------------------------------------- #
def dir_path(work_dir):
    if path.isdir(work_dir):
        return work_dir
    else:
        raise argparse.ArgumentTypeError(f"Directory '{work_dir}' does not exist!")

def ip_validator(ip_str):
    try:
        ipaddress.ip_address(ip_str)
        return ip_str
    except:
        raise argparse.ArgumentTypeError(f"Invalid IP address: {ip_str}!")

def interface_validator(interface):
    try:
        adapters = ifaddr.get_adapters()
        for adapter in adapters:
            name = adapter.name.replace('{', '').replace('}', '')
            nice_name = adapter.nice_name.replace('{', '').replace('}', '')
            if name == interface or nice_name == interface:
                return name
    except:
        pass
    raise argparse.ArgumentTypeError(f"Invalid network interface: {interface}!")

def parse_arguments():
    # Custom parser with targeted epilog behavior
    class MSStyleArgumentParser(argparse.ArgumentParser):
        def __init__(self, *args, **kwargs):
            super().__init__(*args, **kwargs)
            self._is_top_level = False
            self._is_subparser = False
            self._ms_show_epilog_once = False   # only for "missing server type"
            self._error_hint = ""               # printed ONLY on subparser errors

        def error(self, message):
            # Always show the relevant usage
            self.print_usage(sys.stderr)

            # Decide what footer to show
            epilog = ""
            # Only show the top-level epilog if we explicitly asked for it (missing server type)
            if self._is_top_level and self._ms_show_epilog_once and self.epilog:
                try:
                    epilog_text = self.epilog % {"prog": self.prog}
                except Exception:
                    epilog_text = self.epilog
                epilog = "\n\n" + epilog_text + "\n"

            # For subparser errors, append a concise per-server hint (not epilog, so help -h stays clean)
            hint = ""
            if self._is_subparser and self._error_hint:
                try:
                    hint = "\n\n" + (self._error_hint % {"prog": self.prog}) + "\n"
                except Exception:
                    hint = "\n\n" + self._error_hint + "\n"

            # Reset one-shot flag
            self._ms_show_epilog_once = False

            self.exit(2, f"{self.prog}: error: {message}{epilog}{hint}")

        def error_with_epilog(self, message):
            self._ms_show_epilog_once = True
            self.error(message)

    class SdsFormatter(argparse.RawTextHelpFormatter):
        def __init__(self, prog, **kwargs):
            super().__init__(prog, max_help_position=41, **kwargs)
        def _format_action_invocation(self, action):
            if not action.option_strings or action.nargs == 0:
                return super()._format_action_invocation(action)
            opts = ', '.join(action.option_strings)
            if action.metavar:
                return f"{opts} {action.metavar}"
            return opts
    formatter = lambda prog: SdsFormatter(prog)

    # Top-level epilog shown with -h (keyboard input, examples, server type help)
    top_epilog = (
        "keyboard input (while running):\n"
        "  R/r  start recording      P/p  start playback\n"
        "  S/s  stop rec/play        X/x  terminate server\n"
        "  A-H  set user flags 0-7   a-h  clear user flags 0-7\n"
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
    parser = MSStyleArgumentParser(
        formatter_class=formatter,
        add_help=False,
        description=(
            "SDSIO-Server: record and playback SDS data stream files over USB, socket, or serial interface.\n"
            "Configure via *.sdsio.yml file or specify the interface parameters directly on the command line."
        ),
        epilog=top_epilog,
    )
    parser._is_top_level = True
    parser.usage = "%(prog)s [-h] [-V] [{socket | serial | usb} [if-opts]] [-c sdsio.yml] [general-opts]"

    options = parser.add_argument_group("options")
    options.add_argument("--help", "-h", help="Show this help message", action="help")
    options.add_argument("--version", "-V", action="version",
                         help="Show program's version number", version=f"%(prog)s {SDSIO_SERVER_VERSION}")

    # Subparsers
    subparsers = parser.add_subparsers(
        dest="server_type",
        title="interface (optional, default: usb; overrides interface in *.sdsio.yml)",
        metavar="{socket | serial | usb}",
        parser_class=MSStyleArgumentParser
    )

    configuration = parser.add_argument_group("configuration")
    configuration.add_argument("--control", "-c", dest="ctrl_yml", metavar="<*.sdsio.yml>",
                        help="Configure interface, SDS file directories, and playback steps", required=False)

    def _add_general_opts(p):
        """Append a general-opts group to subparser p (called after server-specific groups)."""
        g = p.add_argument_group("general-opts")
        g.add_argument("--playback", "-p", dest="auto_playback", action="store_true",
                       help="Start SDSIO-Server in playback mode (used in CI tests)",
                       default=argparse.SUPPRESS)
        g.add_argument("--workdir", dest="work_dir", metavar="<path>",
                       help="Directory for SDS files (overrides *.sdsio.yml; default: current directory)",
                       type=dir_path, default=argparse.SUPPRESS)
        g.add_argument("--mon-port", "-m", dest="monitor_port", metavar="<port>",
                       help="Monitor control interface port", type=int, default=argparse.SUPPRESS)
        g.add_argument("--log", "-l", dest="log_file", metavar="<file>",
                       help="Redirect console output to a log file (for CI use)",
                       default=argparse.SUPPRESS)
        g.add_argument("--verbose", "-v", action="store_true",
                       help="Enable debug messages", default=argparse.SUPPRESS)
        g.add_argument("--high-priority", dest="high_priority",
                       help="Increase process priority for USB server (requires elevated privileges)",
                       action="store_true", default=argparse.SUPPRESS)

    # socket
    parser_socket = subparsers.add_parser(
        "socket",
        prog=f"{parser.prog} socket",
        formatter_class=formatter,
        help="Run TCP socket server",
        epilog="",  # keep subcommand help clean
    )
    parser_socket._is_subparser = True
    parser_socket._error_hint = "For help on how to use the socket server and its arguments, run: %(prog)s -h"
    parser_socket.usage = "%(prog)s [-h] [--ipaddr <IP> | --netif <Interface>] [--port <TCP Port>] [general-opts]"

    socket_group = parser_socket.add_argument_group("if-opts (optional)")
    socket_group.add_argument("--ipaddr", dest="ip", metavar="<IP>",
                              help="Server IP address (example: 192.168.0.100), cannot be used with 'netif'",
                              type=ip_validator, default=None)
    socket_group.add_argument("--netif", dest="interface", metavar="<Interface>",
                              help="Network interface (example: eth0), cannot be used with 'ipaddr'",
                              type=interface_validator, default=None)
    socket_group.add_argument("--port", dest="port", metavar="<TCP Port>",
                              help="TCP port number (default: 5050)", type=int, default=5050)
    _add_general_opts(parser_socket)

    # serial
    parser_serial = subparsers.add_parser(
        "serial",
        prog=f"{parser.prog} serial",
        formatter_class=formatter,
        help="Run serial server",
        epilog="",  # keep subcommand help clean
    )
    parser_serial._is_subparser = True
    parser_serial._error_hint = "For help on how to use the serial server and its arguments, run: %(prog)s -h"
    parser_serial.usage = "%(prog)s [-h] --port <Serial Port> [--baudrate <Baudrate>] [--parity <Parity>] [--stopbits <Stop bits>] [--connect-timeout <Timeout>] [general-opts]"

    serial_required = parser_serial.add_argument_group("if-opts (required)")
    serial_required.add_argument("--port", dest="port", metavar="<Serial Port>",
                                 help="Serial port (required)", required=True)

    serial_optional = parser_serial.add_argument_group("if-opts (optional)")
    serial_optional.add_argument("--baudrate", dest="baudrate", metavar="<Baudrate>",
                                 help="Baudrate (default: 115200)", type=int, default=115200)
    parity_help = "Parity: none, even, odd, mark, space (default: none)"
    serial_optional.add_argument("--parity", dest="parity", metavar="<Parity>",
                                 choices=list(PARITY_NAME_MAP.keys()), help=parity_help, default='none')
    stopbits_help = (f"Stop bits: {serial.STOPBITS_ONE}, {serial.STOPBITS_ONE_POINT_FIVE}, "
                     f"{serial.STOPBITS_TWO} (default: {serial.STOPBITS_ONE})")
    serial_optional.add_argument("--stopbits", dest="stop_bits", metavar="<Stop bits>",
                                 type=float,
                                 choices=[serial.STOPBITS_ONE, serial.STOPBITS_ONE_POINT_FIVE, serial.STOPBITS_TWO],
                                 help=stopbits_help, default=serial.STOPBITS_ONE)
    serial_optional.add_argument("--connect-timeout", dest="connect_timeout", metavar="<Timeout>",
                                 help="Serial port connection timeout in seconds (default: no timeout)",
                                 type=float, default=None)
    _add_general_opts(parser_serial)

    # usb
    parser_usb = subparsers.add_parser(
        "usb",
        prog=f"{parser.prog} usb",
        formatter_class=formatter,
        help="Run USB bulk server",
        epilog="",  # keep subcommand help clean
    )
    parser_usb._is_subparser = True
    parser_usb._error_hint = "For help on how to use the usb server and its arguments, run: %(prog)s -h"
    parser_usb.usage = "%(prog)s [-h] [general-opts]"
    _add_general_opts(parser_usb)

    general = parser.add_argument_group("general-opts")
    general.add_argument("--playback", "-p", dest="auto_playback", action="store_true",
                         help="Start SDSIO-Server in playback mode (used in CI tests)", default=None)
    general.add_argument("--workdir", dest="work_dir", metavar="<path>",
                        help="Directory for SDS files (overrides *.sdsio.yml; default: current directory)", type=dir_path, default=None)
    general.add_argument("--mon-port", "-m", dest="monitor_port", metavar="<port>",
                        help="Monitor control interface port", type=int, default=None)
    general.add_argument("--log",     "-l", dest="log_file", metavar="<file>",
                        help="Redirect console output to a log file (for CI use)", default=None)
    general.add_argument("--verbose", "-v", action="store_true", help="Enable debug messages")
    general.add_argument("--high-priority", dest="high_priority",
                        help="Increase process priority for USB server (requires elevated privileges)", action="store_true", default=False)


    # two-phase parse
    argv = sys.argv[1:]

    # 1) Parse top-level to get the server_type (don't print epilog here unless missing)
    try:
        top_ns, _ = parser.parse_known_args(argv)
    except SystemExit:
        # top-level parse errors like "invalid choice" -> no epilog (as desired)
        raise

    # Missing server type
    if top_ns.server_type is None:
        if top_ns.ctrl_yml is None:
            # Set default server type to "usb" if not specified in CLI or YAML
            top_ns.server_type = "usb"
    else:
        # 2) Hand EXACT user tokens to the chosen subparser (everything after the server type token)
        try:
            i = argv.index(top_ns.server_type)
        except ValueError:
            # Fallback: if not found for some reason, let subparser try the remainder
            i = 0
        sub_args = argv[i + 1:]
        subparser = subparsers.choices[top_ns.server_type]

        # parse_known_args so opts already handled by the top-level parser
        # (e.g. -c) placed before the server type token are silently ignored
        sub_ns, _ = subparser.parse_known_args(sub_args)

        # Manual mutual-exclusion for socket so both options appear in one group
        if top_ns.server_type == "socket" and sub_ns.ip is not None and sub_ns.interface is not None:
            subparser.error("options --ipaddr and --interface are mutually exclusive.")

        # Merge namespaces (global + subcommand) for downstream use
        for k, v in vars(sub_ns).items():
            setattr(top_ns, k, v)
    return top_ns

async def main():
    args = parse_arguments()

    # configure logging
    if args.verbose:
        fmt = logging.Formatter("%(asctime)s [%(threadName)s] %(levelname)s: %(message)s")
        log = safe_print(level=logging.DEBUG, formatter=fmt)
    else:
        fmt = logging.Formatter("%(message)s")
        log = safe_print(level=logging.INFO, formatter=fmt)
    # override global printer
    global printer
    printer = log

    # Open and parse the control YAML if provided, and apply any configuration
    # CLI arguments will override YAML settings where applicable (e.g. server type)
    ctrl_data = {}
    if args.ctrl_yml:
        ctrl_yml_path = os.path.abspath(args.ctrl_yml)
        try:
            with open(ctrl_yml_path, 'r') as yml_file:
                yml_data = yaml.safe_load(yml_file)
            if 'sdsio' in yml_data:
                ctrl_data = yml_data['sdsio']
            else:
                raise ValueError("Invalid control YAML.")
        except Exception as e:
            printer.error(f"Failed to load control YAML: {e}")

    # Server type
    if args.server_type is not None:
        # Server configuration from CLI arguments (overrides YAML)
        server_type = args.server_type
        if server_type == "socket":
            interface = args.interface
            ip = args.ip
            port = args.port
        elif server_type == "serial":
            port = args.port
            baudrate = args.baudrate
            parity = PARITY_NAME_MAP[args.parity]
            stop_bits = args.stop_bits
            connect_timeout = args.connect_timeout
        elif server_type == "usb":
            high_priority = args.high_priority
    else:
        # Server type from YAML (fallback if not specified in CLI)
        # The interface type is determined by which subnode is present: usb, serial, or socket
        iface_node = ctrl_data.get('interface', None)
        if iface_node and 'serial' in iface_node:
            server_type = 'serial'
            iface_cfg = iface_node['serial'] or {}
        elif iface_node and 'socket' in iface_node:
            server_type = 'socket'
            iface_cfg = iface_node['socket'] or {}
        else:
            server_type = 'usb'  # default
            iface_cfg = (iface_node.get('usb') or {}) if iface_node else {}
        if server_type == "socket":
            socket_iface = iface_cfg.get('netif', None)
            ip = iface_cfg.get('ipaddr', None)
            port = iface_cfg.get('port', 5050)
        elif server_type == "serial":
            port = iface_cfg.get('port')
            baudrate = iface_cfg.get('baudrate', 115200)
            parity = PARITY_NAME_MAP.get(str(iface_cfg.get('parity', 'none')).lower(), serial.PARITY_NONE)
            stop_bits = iface_cfg.get('stopbits', serial.STOPBITS_ONE)
            connect_timeout = None  # YAML config does not support connect timeout
        elif server_type == "usb":
            high_priority = iface_cfg.get('high_priority', False)

    # Working directory
    if args.work_dir:
        work_dir = args.work_dir
    else:
        work_dir = ctrl_data.get('workdir', os.getcwd()) if ctrl_data else os.getcwd()
    if not path.isdir(work_dir):
        raise ValueError("Working directory does not exist!")

    # Auto playback
    auto_playback = args.auto_playback if args.auto_playback else False

    # Playback list
    play_list: Optional[list] = ctrl_data.get('play', None) if ctrl_data else None

    manager = sdsio_manager(work_dir=work_dir, auto_playback=auto_playback, play_list=play_list, mon_port = args.monitor_port)

    try:
        if server_type == "socket":
            if not ip and socket_iface:
                adapters = ifaddr.get_adapters()
                for adapter in adapters:
                    if adapter.name == socket_iface or adapter.nice_name == socket_iface:
                        for ip_info in adapter.ips:
                            try:
                                socket.inet_pton(socket.AF_INET, ip_info.ip)
                                ip = ip_info.ip
                                break
                            except:
                                continue
                    if ip:
                        break
            if not ip:
                ip = socket.gethostbyname(socket.gethostname())
            await sdsio_server_socket_run_supervised(ip, port, manager)

        elif server_type == "serial":
            sdsio_server_serial_run_supervised(port, baudrate, parity,
                                               stop_bits, connect_timeout, manager)

        elif server_type == "usb":
            loop = asyncio.get_running_loop()
            srv = sdsio_server_usb(manager, loop, high_priority=high_priority)
            printer.info("Starting USB Server...")
            await srv.start()

    except KeyboardInterrupt:
        pass

    finally:
        # Shutdown the UI thread and clean-up all SDS streams on exit
        manager._shutdown()

if __name__ == "__main__":
    # minimal printer until main() configures it
    printer = safe_print()
    printer.info("Press Ctrl+C to exit.")
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        printer.info("Ctrl+C received, shutting down.")
    printer.info("Server stopped.")
