# Copyright (c) 2025 Arm Limited. All rights reserved.
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

# Virtual Streaming Interface instance 3 Python script

##@addtogroup arm_vsi_py
#  @{
#
##@package arm_vsi3
#Documentation for VSI peripherals module.
#
#More details.

import os
import logging
from os import path
from typing import List, Optional, Tuple
import yaml

logger = logging.getLogger(__name__)

# Add custom APP
APP_LEVEL = 45
logging.addLevelName(APP_LEVEL, "APP")

## Set verbosity level
#verbosity = logging.DEBUG
#verbosity = logging.INFO
#verbosity = logging.WARNING
verbosity = logging.ERROR
#verbosity = APP_LEVEL

def app(self, message, *args, **kwargs):
    if self.isEnabledFor(APP_LEVEL):
        self._log(APP_LEVEL, message, args, **kwargs)
logging.Logger.app = app

level = { 10: "DEBUG",  20: "INFO", 30: "WARNING", 40: "ERROR", 45: "APP" }
logging.basicConfig(format='%(message)s', level = verbosity, filename = 'sdsio.log', filemode = 'w')

# sdsio.log created by arm_vsi3.py
logger.app(f"Created by {path.abspath(__file__)}\n")


# IRQ registers
IRQ_Status = 0

# Timer registers
Timer_Control  = 0
Timer_Interval = 0

# Timer Control register definitions
Timer_Control_Run_Msk      = 1<<0
Timer_Control_Periodic_Msk = 1<<1
Timer_Control_Trig_IRQ_Msk = 1<<2
Timer_Control_Trig_DMA_Msk = 1<<3

# DMA registers
DMA_Control = 0

# DMA Control register definitions
DMA_Control_Enable_Msk    = 1<<0
DMA_Control_Direction_Msk = 1<<1
DMA_Control_Direction_P2M = 0<<1
DMA_Control_Direction_M2P = 1<<1

# User registers
COMMAND     = 0     # index=0, user read/write
STREAM_ID   = 0     # index=1, user read/write
ARGUMENT    = 0     # index=2, user read/write

# Data buffer
Data = bytearray()

# 'No limit' for idx-end
NO_LIMIT = 0x7FFFFFFF

class IndexAllocator:
    """
    Hands out the next SDS index.
    """

    def __init__(self, idx_start: int = 0, idx_end: int = NO_LIMIT, idx_list: Optional[List[int]] = None):
        self._idx_start = idx_start
        self._idx_end   = idx_end
        self._idx_list  = idx_list
        self._idx_next  = idx_start   # next sequential candidate
        self._list_pos  = 0           # position within _list mode

    def reset(self) -> None:
        self._idx_next = self._idx_start
        self._list_pos  = 0

    def get_idx(self) -> Optional[int]:
        idx = None

        if self._idx_list:
            if self._list_pos < len(self._idx_list):
                idx = self._idx_list[self._list_pos]
                self._list_pos += 1
                return idx
        else:
            # sequential mode
            if self._idx_next <= self._idx_end:
                idx = self._idx_next
                self._idx_next += 1
        return idx

class StreamManager:
    """
    Manages streaming data and associated resources.
    """

    def __init__(self, work_dir: str = None):
        # Initialize the StreamManager instance.
        self._stream_id = 0
        base_dir = work_dir if work_dir else os.getcwd()
        self._opened_streams = {}           # sid -> (file_obj, name, mode)
        self._stream_indexes = {}           # stream name -> IndexAllocator

        # Load SDSIO configuration from sdsio.yml configuration file
        dir_path, idx_start, idx_end, idx_list = self._load_sdsio_config(base_dir)
        self._sds_dir   = dir_path          # directory that stores SDS files
        self._idx_start = idx_start
        self._idx_end   = idx_end
        self._idx_list  = idx_list
        self._rec_play  = {1: "Record:   ", 0: "Playback: "}

    def _format_stream_path(self, fname) -> str:
        try:
            base_dir = os.getcwd()
            p = os.path.relpath(fname, base_dir)
            # If file is outside base_dir, relpath starts with '..' (e.g., '../foo' or '..\foo')
            if p == os.pardir or p.startswith(os.pardir + os.sep):
                return os.path.abspath(fname)
        except Exception:
            # Different drives on Windows or other relpath issues -> use absolute
            return fname

        # p is relative here; add ./ or .\ for clarity if not already present
        if p.startswith("./") or p.startswith(".\\"):
            return p
        return (".\\" if os.sep == "\\" else "./") + p

    @staticmethod
    def _load_sdsio_config(base_dir: str) -> Tuple[str, int, int, Optional[List[int]]]:
        """
        Load SDSIO config from sdsio.yml/.yaml in workdir.

        dir_path: absolute directory path (str)          [default: workdir]
        idx_start: int                                   [default: 0]
        idx_end: int                                     [default: NO_LIMIT]
        idx_list: [List[int]]                            [default: None]
        """
        workdir_abs = path.abspath(base_dir)
        cfg_path = None
        for name in ("sdsio.yml", "sdsio.yaml"):
            candidate = path.join(workdir_abs, name)
            if path.isfile(candidate):
                cfg_path = candidate
                break

        # defaults
        dir_path:  str = workdir_abs
        idx_start: int = 0
        idx_end:   int = NO_LIMIT
        idx_list:  Optional[List[int]] = None

        if cfg_path is None:
            logger.warning("sdsio.yml: No configuration file. Default values will be used.")
            return dir_path, idx_start, idx_end, idx_list

        with open(cfg_path, "r", encoding="utf-8") as f:
            try:
                data = yaml.safe_load(f) or {}
            except:
                logger.warning(f"sdsio.yml: Failed to parse '{path.basename(cfg_path)}'. Default values will be used.")
                data = {}

        # dir
        if "dir" in data and data["dir"] is not None:
            if not isinstance(data["dir"], str):
                logger.warning(f"sdsio.yml: 'dir' must be a string (path). Default value will be used: {dir_path}.")
            d = data["dir"].strip()
            if d:
                dir_path_abs = path.normpath(d if path.isabs(d) else path.abspath(path.join(workdir_abs, d)))
                if not path.isdir(dir_path_abs):
                    # Check if dir_path_abs is a subfolder of workdir_abs
                    try:
                        rel = os.path.relpath(dir_path_abs, workdir_abs)
                        is_subfolder = not rel.startswith(os.pardir + os.sep) and not os.path.isabs(rel)
                    except Exception:
                        is_subfolder = False

                    if is_subfolder:
                        try:
                            os.makedirs(dir_path_abs, exist_ok=True)
                            dir_path = dir_path_abs
                            logger.info(f"sdsio.yml: Directory '{dir_path_abs}' did not exist and was created.")
                        except Exception:
                            logger.warning(f"sdsio.yml: Failed to create directory '{dir_path_abs}'. Default value will be used: {dir_path}.")
                    else:
                        logger.warning(f"sdsio.yml: Directory '{dir_path_abs}' does not exist. Default value will be used: {dir_path}.")
                else:
                    dir_path = dir_path_abs


        # idx-start
        if "idx-start" in data and data["idx-start"] is not None:
            v = data["idx-start"]
            if isinstance(v, bool):
                logger.warning(f"sdsio.yml: 'idx-start' must be an integer, got boolean. Default value will be used: {idx_start}.")
            else:
                try:
                    idx_start = int(v)
                except:
                    logger.warning(f"sdsio.yml: 'idx-start' must be an integer. Default value will be used: {idx_start}.")
                if idx_start < 0:
                    logger.warning(f"sdsio.yml: 'idx-start' must be >= 0. Default value will be used: {idx_start}.")

        # idx-end
        if "idx-end" in data and data["idx-end"] is not None:
            v = data["idx-end"]
            if isinstance(v, bool):
                logger.warning(f"sdsio.yml: 'idx-end' must be an integer, got boolean. Default value will be used: {idx_end}.")
            else:
                try:
                    idx_end = int(v)
                except:
                    logger.warning(f"sdsio.yml: 'idx-end' must be an integer. Default value will be used: {idx_end}.")
                if idx_end < 0:
                    logger.warning(f"sdsio.yml: 'idx-end' must be >= 0. Default value will be used: {idx_end}.")
                if idx_end < idx_start:
                    logger.warning(f"sdsio.yml: 'idx-end' must be >= idx_start. Default value will be used: {idx_end}.")

        # idx-list
        if "idx-list" in data and data["idx-list"] is not None:
            lst_err = False
            lst = data["idx-list"]
            if not isinstance(lst, (list, tuple)):
                logger.warning(f"sdsio.yml: 'idx-list' must be a list of integers. idx_start {idx_start} and idx_end {idx_end} will be used.")
                lst_err = True
            out: List[int] = []
            seen = set()
            for i, v in enumerate(lst):
                if isinstance(v, bool):
                    logger.warning(f"sdsio.yml: 'idx-list[{i}]' must be an integer, got boolean. idx_start {idx_start} and idx_end {idx_end} will be used.")
                    lst_err = True
                    break
                try:
                    iv = int(v)
                except:
                    logger.warning(f"sdsio.yml: 'idx-list[{i}]' must be an integer. idx_start {idx_start} and idx_end {idx_end} will be used.")
                    lst_err = True
                    break
                if iv not in seen:
                    out.append(iv)
                    seen.add(iv)
            if not lst_err and out:
                idx_list = out

        return dir_path, idx_start, idx_end, idx_list

    def open(self, name: str, mode: int) -> int:
        # Open a stream in read (mode=0) or write (mode=1).
        # Returns a non-zero stream ID on success, or 0 on error.
        logger.debug(f"Open name={name}, mode={mode}")

        # Check mode
        if mode not in (0, 1):
            logger.app(f"ERROR:    Open failed. Invalid mode for stream: {name}.")
            return 0

        try:
            if not name or any(ch in name for ch in '"*/:<>?\\|'):
                logger.app(f"{self._rec_play[mode]} {name} - ERROR. Invalid name.")
                return 0
            if name in (n for (_, n, _) in self._opened_streams.values()):
                logger.app(f"{self._rec_play[mode]} {name} - ERROR. Already open.")
                return 0

            # Check if stream name already has an IndexAllocator, else create one
            if name not in self._stream_indexes:
                # Use the loaded config for all streams for now
                self._stream_indexes[name] = IndexAllocator(self._idx_start, self._idx_end, self._idx_list)

            # Get new index
            idx = self._stream_indexes[name].get_idx()
            if idx is None:
                logger.app(f"{self._rec_play[mode]} {name} - ACCESS REJECTED. Index excluded in sdsio.yml")
                return 0

            fname = path.join(self._sds_dir, f"{name}.{idx}.sds")
            file_path = self._format_stream_path(fname)

            if mode == 1:
                # write mode
                f = open(fname, 'wb')
                self._stream_id += 1
                sid = self._stream_id
                logger.app(f"{self._rec_play[mode]} {name} ({file_path}).")

            else:
                # read mode:
                if path.exists(fname):
                    f = open(fname, 'rb')
                    logger.app(f"{self._rec_play[mode]} {name} ({file_path}).")
                    self._stream_id += 1
                    sid = self._stream_id
                else:
                    sid = 0

            if sid != 0:
                self._opened_streams[sid] = (f, name, mode)
            return sid

        except Exception:
            logger.app(f"{self._rec_play[mode]} {name} - ERROR. Failed to open.")
            return 0

    def close(self, sid: int) -> bool:
        # Close the stream with the given ID.
        # Returns True on success, False if invalid ID.
        entry = self._opened_streams.pop(sid, None)
        if not entry:
            logger.app(f"ERROR:    Close failed. Invalid stream ID: {sid}.")
            return False
        f, name, mode = entry
        try:
            f.close()
            logger.app(f"Closed:    {name}.")
            return True
        except Exception:
            logger.app(f"{self._rec_play[mode]} {name} - ERROR. Failed to close.")
            return False

    def write(self, sid: int, data: bytes) -> bool:
        # Write raw bytes to an open write stream.
        # Returns True on success, False otherwise.
        entry = self._opened_streams.get(sid)
        if not entry:
            logger.app(f"ERROR:    Write failed. Invalid stream ID: {sid}.")
            return False
        f, name, mode = entry
        if mode != 1:
            logger.app(f"{self._rec_play[mode]} {name} - ERROR. Not opened for write.")
            return False
        try:
            f.write(data)
            f.flush()
            return True
        except Exception:
            logger.app(f"{self._rec_play[mode]} {name} - ERROR. Failed to write.")
            return False

    def read(self, sid: int, size: int) -> tuple[bytes, bool]:
        # Read up to `size` bytes from an open read stream.
        # Returns a tuple (data, eof) where `eof` is True if end-of-stream reached.
        entry = self._opened_streams.get(sid)
        if not entry:
            logger.app(f"ERROR:    Read failed. Invalid stream ID: {sid}.")
            return b'', True
        f, name, mode = entry
        if mode != 0:
            logger.app(f"{self._rec_play[mode]} {name} - ERROR. Not opened for read.")
            return b'', True
        try:
            data = f.read(size)
            eof = not data
            return data, eof
        except Exception:
            logger.app(f"{self._rec_play[mode]} {name} - ERROR. Failed to read.")
            return b'', True

    def clean(self):
        # Close all open streams.
        for sid in list(self._opened_streams):
            self.close(sid)

# Global instance using current directory
Stream = StreamManager()

## Process command
#  @param command requested SDSIO command
def processCOMMAND(command):
    global Data, Stream, STREAM_ID, ARGUMENT

    cmd = { 1: "CMD_OPEN", 2: "CMD_CLOSE", 3: "CMD_WRITE", 4: "CMD_READ" }

    if not command in cmd:
        logger.app(f"ERROR:    Unknown COMMAND: {command}.")
        return 0

    logger.info(f"Processing {cmd[command]}")

    if command == 1: # Open
        fname = Data[:Data.find(0)].decode("utf-8")
        STREAM_ID = Stream.open(fname, ARGUMENT)

    elif command == 2: # Close
        Stream.close(STREAM_ID)

    elif command == 3: # Write
        # Return data length or SDSIO_ERROR
        ARGUMENT = len(Data) if Stream.write(STREAM_ID, Data) else -1

    elif command == 4: # Read
        # Return data length or SDSIO_EOS
        Data, eof = Stream.read(STREAM_ID, ARGUMENT)
        ARGUMENT = len(Data) if not eof else -6

    return command


## Initialize
#  @return None
def init():
    logger.info("Python function init() called")


## Read interrupt request (the VSI IRQ Status Register)
#  @return value value read (32-bit)
def rdIRQ():
    global IRQ_Status
    logger.info("Python function rdIRQ() called")

    value = IRQ_Status
    logger.debug(f"Read interrupt request: {value}")

    return value


## Write interrupt request (the VSI IRQ Status Register)
#  @param value value to write (32-bit)
#  @return value value written (32-bit)
def wrIRQ(value):
    global IRQ_Status
    logger.info("Python function wrIRQ() called")

    IRQ_Status = value
    logger.debug(f"Write interrupt request: {value}")

    return value


## Write Timer registers (the VSI Timer Registers)
#  @param index Timer register index (zero based)
#  @param value value to write (32-bit)
#  @return value value written (32-bit)
def wrTimer(index, value):
    global Timer_Control, Timer_Interval
    logger.info("Python function wrTimer() called")

    if index == 0:
        Timer_Control = value
        logger.debug(f"Write Timer_Control: {value}")
    elif index == 1:
        Timer_Interval = value
        logger.debug(f"Write Timer_Interval: {value}")

    return value


## Timer event (called at Timer Overflow)
#  @return None
def timerEvent():
    logger.info("Python function timerEvent() called")


## Write DMA registers (the VSI DMA Registers)
#  @param index DMA register index (zero based)
#  @param value value to write (32-bit)
#  @return value value written (32-bit)
def wrDMA(index, value):
    global DMA_Control
    logger.info("Python function wrDMA() called")

    if index == 0:
        DMA_Control = value
        logger.debug(f"Write DMA_Control: {value}")

    return value


## Read data from peripheral for DMA P2M transfer (VSI DMA)
#  @param size size of data to read (in bytes, multiple of 4)
#  @return data data read (bytearray)
def rdDataDMA(size):
    global Data
    logger.info("Python function rdDataDMA() called")

    n = min(len(Data), size)
    data = bytearray(size)
    data[0:n] = Data[0:n]
    logger.debug(f"Read data ({size} bytes)")

    return data


## Write data to peripheral for DMA M2P transfer (VSI DMA)
#  @param data data to write (bytearray)
#  @param size size of data to write (in bytes, multiple of 4)
#  @return None
def wrDataDMA(data, size):
    global Data
    logger.info("Python function wrDataDMA() called")

    Data = data
    logger.debug(f"Write data ({size} bytes)")

    return


## Read user registers (the VSI User Registers)
#  @param index user register index (zero based)
#  @return value value read (32-bit)
def rdRegs(index):
    global Timer_Control, COMMAND, STREAM_ID, ARGUMENT
    logger.info("Python function rdRegs() called")

    if   index == 0:
        value = COMMAND
        logger.debug(f"Read COMMAND: {value}")
    elif index == 1:
        value = STREAM_ID
        logger.debug(f"Read STREAM_ID: {value}")
    elif index == 2:
        value = ARGUMENT
        logger.debug(f"Read ARGUMENT: {value}")
    else:
        logger.debug(f"User register {index} not used")
        value = 0

    return value


## Write user registers (the VSI User Registers)
#  @param index user register index (zero based)
#  @param value value to write (32-bit)
#  @return value value written (32-bit)
def wrRegs(index, value):
    global COMMAND, STREAM_ID, ARGUMENT
    logger.info("Python function wrRegs() called")

    if   index == 0:
        COMMAND = processCOMMAND(value)
    elif index == 1:
        STREAM_ID = value
        logger.debug(f"Write STREAM_ID: {value}")
    elif index == 2:
        ARGUMENT = value
        logger.debug(f"Write ARGUMENT: {value}")
    else:
        logger.debug(f"User register {index} not used")

    return value


## @}

