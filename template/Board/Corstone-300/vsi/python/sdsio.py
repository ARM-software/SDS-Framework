# Copyright (c) 2025-2026 Arm Limited. All rights reserved.
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

import os
import os.path as path
import threading
import time
import logging
from typing import Optional, NamedTuple

logger = logging.getLogger("sdsio")

# ---------------------------------------------------------------------------- #
#                  SDSIO server-compatible stream implementation                #
# ---------------------------------------------------------------------------- #
SDSIO_VSI_VERSION = "3.0.0"

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

# SDS Flags bit positions
SDS_FLAG_MASK_START        = (1 << 31)
SDS_FLAG_MASK_CI_TERMINATE = (1 << 30)
SDS_FLAG_MASK_PLAYBACK_MODE= (1 << 29)
SDS_FLAG_MASK_ALIVE        = (1 << 28)
SDS_FLAG_MASK_RESET        = (1 << 27)
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
#                            SDS IO Manager                                    #
# ---------------------------------------------------------------------------- #
class sdsio_manager:
    def __init__(
        self,
        work_dir,
        auto_playback=False,
        play_list: Optional[list] = None,
        mon_port: Optional[int] = None,
        write_flush_records: Optional[int] = None,
        status_bar_factory=None,
        monitor_factory=None,
        control_input_factory=None,
    ):
        self._stream_id = 0
        self._play_step_index = 0
        self._rec_index = None      # recording session index (None = not yet determined)
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
        self._write_flush_records = write_flush_records
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
                logger.info("SDSIO command input: R=Record, P=playback, S/s=stop, T/t=reset, X/x=exit, A-H=set flags 0-7, a-h=clear flags 0-7.")
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

    def _format_path(self, p: str) -> str:
        """Return path relative to work_dir when it doesn't escape it, else absolute."""
        try:
            _rel = path.relpath(p, self._work_dir)
            if not _rel.startswith('..'):
                return _rel
        except ValueError:
            pass  # different drives on Windows
        return path.abspath(p)

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
                # timestamp_boundaries is generated when playback file is opened
                while not self._label_list or not self._timestamp_boundaries:
                    if stop_evt.is_set() or buf.eof:
                        return
                    time.sleep(0.1)

            _stream = self.opened_streams[sid]
            for _index, _sds_file_path in enumerate(_stream.file_paths):
                if stop_evt.is_set():
                    break

                # Check if sds file exists. If so, rename it to *.bak
                if path.exists(_sds_file_path):
                    if path.exists(_sds_file_path + ".bak"):
                        try:
                            os.remove(_sds_file_path + ".bak")
                        except Exception:
                            logger.warning(f"Could not delete backup file '{self._format_path(_sds_file_path)}.bak'.")
                    try:
                        os.rename(_sds_file_path, _sds_file_path + ".bak")
                    except Exception:
                        logger.warning(f"Could not create backup for existing file '{self._format_path(_sds_file_path)}'")

                _eof_reached = False
                with open(_sds_file_path, "wb") as _file_obj:
                    _records_since_flush = 0
                    if _index > 0:
                        # First file open was already notified in _open(); notify for subsequent files here
                        logger.info(f"Record:   {_stream.name} ({self._format_path(_sds_file_path)})")
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
                                # Incomplete header at EOF - discard fragment and stop
                                _data = bytearray()
                                _eof_reached = True
                                break
                            elif stop_evt.is_set():
                                # Read timed out and stream was closed - no more data expected
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
                                    break  # keep data - it will be written to the next label file

                            _needed = 8 + _data_block_size
                            if _data_sz < _needed:
                                # Accumulate payload bytes
                                _chunk = buf.read(_needed - _data_sz, timeout=0.1)
                                if _chunk:
                                    _data += _chunk
                                elif buf.eof:
                                    # Incomplete record at EOF - discard and stop
                                    _data = bytearray()
                                    _eof_reached = True
                                    break
                                elif stop_evt.is_set():
                                    # Read timed out and stream was closed - no more data expected
                                    _data = bytearray()
                                    _eof_reached = True
                                    break
                                # else: timeout, try again
                            else:
                                # Complete record: write and reset for next record
                                _file_obj.write(_data)
                                _records_since_flush += 1
                                if self._write_flush_records is not None:
                                    if _records_since_flush >= self._write_flush_records:
                                        _file_obj.flush()
                                        os.fsync(_file_obj.fileno())
                                        _records_since_flush = 0
                                _data = bytearray()
                    if self._write_flush_records is not None:
                        _file_obj.flush()
                        os.fsync(_file_obj.fileno())
                # Last file close is handled in close(); send close only for non-last files
                if not _eof_reached and _index < len(_stream.file_paths) - 1:
                    logger.info(f"Closed:   {name} ({self._format_path(_sds_file_path)})")
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
            self._flags.request_auto_playback_start()
        elif self._flags.auto_playback and self._last_playback_stream_name:
            if self._flags.request_auto_playback_terminate():
                logger.info("Playback complete - no more steps remaining.")

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
                        _step_desc = _step.get('step', '')
                        _desc_suffix = f": {_step_desc}" if _step_desc else ""
                        logger.info(f"Playback step {self._play_step_index + 1}/{len(self._play_list)}{_desc_suffix}.")
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
                    # find index for new recording session
                    if self._rec_index is None:
                        # first session: scan from 0 to find first unused index
                        _idx = 0
                        _sds_file_path = self._make_sds_file_path(name, str(_idx), mode)
                        while path.exists(_sds_file_path):
                            _idx += 1
                            _sds_file_path = self._make_sds_file_path(name, str(_idx), mode)
                        self._rec_index = _idx
                    else:
                        # subsequent sessions: increment from previous
                        self._rec_index += 1
                    self._label_list.append(str(self._rec_index))

            _file_paths = self._build_stream_file_paths(name, mode)
            # validate if recdir exsist
            if _file_paths and not path.exists(path.dirname(_file_paths[0])):
                logger.error(f"Recording directory does not exist for stream '{name}': {self._format_path(path.dirname(_file_paths[0]))}")
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
                logger.info(f"Record:   {name} ({self._format_path(_file_paths[0])})")
                if self._monitor:
                    self._monitor.send_open_msg(_file_paths[0], 1)

        else:
            _file_paths = self._build_stream_file_paths(name, mode)
            # Validate that files for all labels exist
            for _sds_file_path in _file_paths:
                if not path.exists(_sds_file_path):
                    logger.error(f"Missing file for playback stream '{name}': {self._format_path(_sds_file_path)}")
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
                logger.info(f"Playback: {name} ({self._format_path(_file_paths[0])})")
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
                    logger.info(f"Closed:   {_name} ({self._format_path(_sds_file_path)})")
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
                    logger.info(f"Closed:   {_name} ({self._format_path(_sds_file_path)})")
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
                            logger.info(f"Closed:   {_stream.name} ({self._format_path(_sds_file_path)})")
                            if self._monitor:
                                self._monitor.send_close_msg(_sds_file_path)
                            self.opened_streams[sid] = self.opened_streams[sid]._replace(file_idx=_f_idx + 1)
                            _next_idx = _f_idx + 1
                            if _next_idx < len(_stream.file_paths):
                                # First file open was already notified in _open(); notify for subsequent files here
                                _sds_file_path = _stream.file_paths[_next_idx]
                                logger.info(f"Playback: {_stream.name} ({self._format_path(_sds_file_path)})")
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
