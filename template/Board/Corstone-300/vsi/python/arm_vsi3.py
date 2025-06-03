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

logger = logging.getLogger(__name__)

## Set verbosity level
#verbosity = logging.DEBUG
#verbosity = logging.INFO
#verbosity = logging.WARNING
verbosity = logging.ERROR

# [debugging] Verbosity settings
level = { 10: "DEBUG",  20: "INFO",  30: "WARNING",  40: "ERROR" }
logging.basicConfig(format='Py: VSI3: [%(levelname)s]\t%(message)s', level = verbosity)
logger.info("Verbosity level is set to " + level[verbosity])


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

class StreamManager:
    def __init__(self, work_dir: str = None):
        # Initialize manager using provided work_dir or current directory if None.
        self.stream_id = 0
        base_dir = work_dir if work_dir else os.getcwd()
        self.work_dir = path.normpath(base_dir)
        self.opened_streams = {}  # sid -> (file_obj, name, mode)

    def open(self, name: str, mode: int) -> int:
        # Open a stream in read (mode=0) or write (mode=1).
        # Returns a non-zero stream ID on success, or 0 on error.
        logger.debug(f"Open name={name}, mode={mode}")
        try:
            if not name or any(ch in name for ch in '"*/:<>?\\|'):
                logger.warning(f"Invalid stream name: {name}")
                return 0
            if name in (n for (_, n, _) in self.opened_streams.values()):
                logger.warning(f"Stream already open: {name}")
                return 0

            if mode == 1:
                # write mode: pick next available index
                idx = 0
                fname = path.join(self.work_dir, f"{name}.{idx}.sds")
                logger.debug(f"Opening stream for writing: {fname}")
                while path.exists(fname):
                    idx += 1
                    fname = path.join(self.work_dir, f"{name}.{idx}.sds")
                f = open(fname, 'wb')
                self.stream_id += 1
                sid = self.stream_id
                logger.debug(f"Opened new stream for writing: {fname}")

            else:
                # read mode: determine index via index file
                index_file = path.join(self.work_dir, f"{name}.index.txt")
                idx = 0
                if path.exists(index_file):
                    try:
                        with open(index_file, 'r') as ix:
                            line = ix.readline().strip()
                            if line.isdigit():
                                idx = int(line)
                    except Exception:
                        logger.warning(f"Could not read index file for {name}")
                fname = path.join(self.work_dir, f"{name}.{idx}.sds")
                if path.exists(fname):
                    f = open(fname, 'rb')
                    logger.debug(f"Opened stream for reading: {fname}")
                    self.stream_id += 1
                    sid = self.stream_id
                else:
                    sid = 0
                # update index for next read
                try:
                    with open(index_file, 'w') as ix:
                        ix.write(str(idx + 1 if path.exists(fname) else 0))
                except Exception:
                    logger.warning(f"Could not update index file for {name}")

            if sid != 0:
                self.opened_streams[sid] = (f, name, mode)
            return sid

        except Exception:
            logger.exception("Failed to open stream")
            return 0

    def close(self, sid: int) -> bool:
        # Close the stream with the given ID.
        # Returns True on success, False if invalid ID.
        entry = self.opened_streams.pop(sid, None)
        if not entry:
            logger.error(f"Invalid stream ID on close: {sid}")
            return False
        f, name, _ = entry
        try:
            f.close()
            return True
        except Exception:
            logger.exception(f"Error closing stream {name} (ID {sid})")
            return False

    def write(self, sid: int, data: bytes) -> bool:
        # Write raw bytes to an open write stream.
        # Returns True on success, False otherwise.
        entry = self.opened_streams.get(sid)
        if not entry or entry[2] != 1:
            logger.error(f"Invalid write operation on stream ID: {sid}")
            return False
        f = entry[0]
        try:
            f.write(data)
            f.flush()
            return True
        except Exception:
            logger.exception(f"Write error on stream ID {sid}")
            return False

    def read(self, sid: int, size: int) -> tuple[bytes, bool]:
        # Read up to `size` bytes from an open read stream.
        # Returns a tuple (data, eof) where `eof` is True if end-of-stream reached.
        entry = self.opened_streams.get(sid)
        if not entry or entry[2] != 0:
            logger.error(f"Invalid read operation on stream ID: {sid}")
            return b'', True
        f = entry[0]
        try:
            data = f.read(size)
            eof = not data or len(data) < size
            return data, eof
        except Exception:
            logger.exception(f"Read error on stream ID {sid}")
            return b'', True

    def clean(self):
        # Close all open streams.
        for sid in list(self.opened_streams):
            self.close(sid)

# Global instance using current directory
Stream = StreamManager()

## Process command
#  @param command requested SDSIO command
def processCOMMAND(command):
    global Data, Stream, STREAM_ID, ARGUMENT

    cmd = { 1: "CMD_OPEN", 2: "CMD_CLOSE", 3: "CMD_WRITE", 4: "CMD_READ" }

    if not command in cmd:
        logger.info(f"Unknown COMMAND: {command}")
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
        logging.debug(f"User register {index} not used")
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
        logging.debug(f"User register {index} not used")

    return value


## @}

