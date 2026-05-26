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
import yaml

from sdsio import (
    CMD_CLOSE,
    CMD_FLAGS,
    CMD_INFO,
    CMD_OPEN,
    CMD_PING,
    CMD_READ,
    CMD_WRITE,
    SDSIO_VSI_VERSION,
    sdsio_manager,
)

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
FLAGS_SET   = 0     # index=3, user read/write
FLAGS_CLR   = 0     # index=4, user read/write

# Data buffer
Data = bytearray()

# SDS I/O error/status codes (32-bit two's-complement, used in the ARGUMENT register)
SDSIO_ERROR = (-1 & 0xFFFFFFFF)  # -1
SDSIO_EOS   = (-7 & 0xFFFFFFFF)  # -7

# VSI3 writes SDSIO logs to sdsio.log
logger = logging.getLogger("sdsio")
logger.setLevel(logging.INFO)
logger.propagate = False
for _handler in logger.handlers[:]:
    logger.removeHandler(_handler)
    _handler.close()
_log_handler = logging.FileHandler("sdsio.log", mode="w", encoding="utf-8")
_log_handler.setFormatter(logging.Formatter("%(message)s"))
logger.addHandler(_log_handler)
logger.info(f"Created by {path.abspath(__file__)}\n")

def _load_sdsio_server_config(base_dir: str):
    _cfg_path = None
    _candidates = [
        path.join(base_dir, "sdsio.yml"),
        path.join(base_dir, "sdsio.yaml"),
    ]
    _candidates.extend(sorted(path.join(base_dir, _name) for _name in os.listdir(base_dir) if _name.endswith(".sdsio.yml")))
    _candidates.extend(sorted(path.join(base_dir, _name) for _name in os.listdir(base_dir) if _name.endswith(".sdsio.yaml")))

    for _candidate in _candidates:
        if path.isfile(_candidate):
            _cfg_path = _candidate
            break

    _ctrl_data = {}
    if _cfg_path:
        try:
            with open(_cfg_path, "r", encoding="utf-8") as _yml_file:
                _yml_data = yaml.safe_load(_yml_file) or {}
            if "sdsio" in _yml_data:
                _ctrl_data = _yml_data["sdsio"] or {}
        except Exception as _e:
            logger.error(f"Failed to load control YAML: {_e}")

    if _ctrl_data:
        logger.info(f"SDSIO configuration YAML: {_cfg_path}")

    _work_dir = _ctrl_data.get("workdir", base_dir) if _ctrl_data else base_dir
    _work_dir = path.normpath(path.join(base_dir, _work_dir)) if not path.isabs(_work_dir) else path.normpath(_work_dir)
    _play_list = _ctrl_data.get("play", None) if _ctrl_data else None

    # auto-playback is always enabled on VSI
    _auto_playback = True
    return _work_dir, _auto_playback, _play_list


def _build_sdsio_request(command: int, sid: int = 0, argument: int = 0, data: bytes = b"") -> bytearray:
    _req = bytearray()
    _req.extend(command.to_bytes(4, "little"))
    _req.extend(sid.to_bytes(4, "little"))
    _req.extend(argument.to_bytes(4, "little"))
    _req.extend(len(data).to_bytes(4, "little"))
    _req.extend(data)
    return _req

logger.info(f"SDSIO VSI version {SDSIO_VSI_VERSION}")
_work_dir, _auto_playback, _play_list = _load_sdsio_server_config(os.getcwd())
Stream = sdsio_manager(
    work_dir=_work_dir,
    auto_playback=_auto_playback,
    play_list=_play_list,
    mon_port=None,
    status_bar_factory=False,
    monitor_factory=False,
    control_input_factory=False,
)


## Process command
#  @param command requested SDSIO command
def processCOMMAND(command):
    global Data, Stream, STREAM_ID, ARGUMENT, FLAGS_SET, FLAGS_CLR

    cmd = { 1: "CMD_OPEN", 2: "CMD_CLOSE", 3: "CMD_WRITE", 4: "CMD_READ", 5: "CMD_PING", 6: "CMD_FLAGS", 7: "CMD_INFO" }

    if not command in cmd:
        logger.error(f"ERROR:    Unknown COMMAND: {command}.")
        return 0

    logger.debug(f"Processing {cmd[command]}")

    try:
        if command == CMD_OPEN:
            _resp = Stream.execute_request(_build_sdsio_request(CMD_OPEN, argument=ARGUMENT, data=Data))
            STREAM_ID = int.from_bytes(_resp[4:8], "little") if len(_resp) >= 8 else 0
            ARGUMENT = int.from_bytes(_resp[8:12], "little") if len(_resp) >= 12 else SDSIO_ERROR

        elif command == CMD_CLOSE:
            Stream.execute_request(_build_sdsio_request(CMD_CLOSE, sid=STREAM_ID))
            ARGUMENT = 0

        elif command == CMD_WRITE:
            _can_write = STREAM_ID in Stream._write_buffers
            Stream.execute_request(_build_sdsio_request(CMD_WRITE, sid=STREAM_ID, data=Data))
            ARGUMENT = len(Data) if _can_write else SDSIO_ERROR

        elif command == CMD_READ:
            _resp = Stream.execute_request(_build_sdsio_request(CMD_READ, sid=STREAM_ID, argument=ARGUMENT))
            _eof = int.from_bytes(_resp[8:12], "little") if len(_resp) >= 12 else 1
            _size = int.from_bytes(_resp[12:16], "little") if len(_resp) >= 16 else 0
            Data = bytearray(_resp[16:16 + _size])
            ARGUMENT = SDSIO_EOS if _eof and _size == 0 else _size

        elif command == CMD_PING:
            _resp = Stream.execute_request(_build_sdsio_request(CMD_PING, sid=STREAM_ID))
            ARGUMENT = int.from_bytes(_resp[8:12], "little") if len(_resp) >= 12 else SDSIO_ERROR

        elif command == CMD_FLAGS:
            _resp = Stream._get_async_flags()
            FLAGS_SET = int.from_bytes(_resp[4:8], "little") if len(_resp) >= 8 else 0
            FLAGS_CLR = int.from_bytes(_resp[8:12], "little") if len(_resp) >= 12 else 0
            Data = bytearray()

        elif command == CMD_INFO:
            Stream.execute_request(_build_sdsio_request(CMD_INFO, sid=STREAM_ID, argument=ARGUMENT, data=Data))
            ARGUMENT = len(Data)

    except Exception:
        logger.exception(f"ERROR:    Failed to process {cmd[command]}.")
        ARGUMENT = SDSIO_ERROR
        if command == CMD_OPEN:
            STREAM_ID = 0
        elif command == CMD_FLAGS:
            FLAGS_SET = 0
            FLAGS_CLR = 0

    return command


## Initialize
#  @return None
def init():
    logger.debug("Python function init() called")


## Read interrupt request (the VSI IRQ Status Register)
#  @return value value read (32-bit)
def rdIRQ():
    global IRQ_Status
    logger.debug("Python function rdIRQ() called")

    value = IRQ_Status
    logger.debug(f"Read interrupt request: {value}")

    return value


## Write interrupt request (the VSI IRQ Status Register)
#  @param value value to write (32-bit)
#  @return value value written (32-bit)
def wrIRQ(value):
    global IRQ_Status
    logger.debug("Python function wrIRQ() called")

    IRQ_Status = value
    logger.debug(f"Write interrupt request: {value}")

    return value


## Write Timer registers (the VSI Timer Registers)
#  @param index Timer register index (zero based)
#  @param value value to write (32-bit)
#  @return value value written (32-bit)
def wrTimer(index, value):
    global Timer_Control, Timer_Interval
    logger.debug("Python function wrTimer() called")

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
    logger.debug("Python function timerEvent() called")


## Write DMA registers (the VSI DMA Registers)
#  @param index DMA register index (zero based)
#  @param value value to write (32-bit)
#  @return value value written (32-bit)
def wrDMA(index, value):
    global DMA_Control
    logger.debug("Python function wrDMA() called")

    if index == 0:
        DMA_Control = value
        logger.debug(f"Write DMA_Control: {value}")

    return value


## Read data from peripheral for DMA P2M transfer (VSI DMA)
#  @param size size of data to read (in bytes, multiple of 4)
#  @return data data read (bytearray)
def rdDataDMA(size):
    global Data
    logger.debug("Python function rdDataDMA() called")

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
    logger.debug("Python function wrDataDMA() called")

    Data = data
    logger.debug(f"Write data ({size} bytes)")

    return


## Read user registers (the VSI User Registers)
#  @param index user register index (zero based)
#  @return value value read (32-bit)
def rdRegs(index):
    global Timer_Control, COMMAND, STREAM_ID, ARGUMENT, FLAGS_SET, FLAGS_CLR
    logger.debug("Python function rdRegs() called")

    if   index == 0:
        value = COMMAND
        logger.debug(f"Read COMMAND: {value}")
    elif index == 1:
        value = STREAM_ID
        logger.debug(f"Read STREAM_ID: {value}")
    elif index == 2:
        value = ARGUMENT
        logger.debug(f"Read ARGUMENT: {value}")
    elif index == 3:
        value = FLAGS_SET
        logger.debug(f"Read FLAGS_SET: {value}")
    elif index == 4:
        value = FLAGS_CLR
        logger.debug(f"Read FLAGS_CLR: {value}")
    else:
        logger.debug(f"User register {index} not used")
        value = 0

    return value


## Write user registers (the VSI User Registers)
#  @param index user register index (zero based)
#  @param value value to write (32-bit)
#  @return value value written (32-bit)
def wrRegs(index, value):
    global COMMAND, STREAM_ID, ARGUMENT, FLAGS_SET, FLAGS_CLR
    logger.debug("Python function wrRegs() called")

    if   index == 0:
        COMMAND = processCOMMAND(value)
    elif index == 1:
        STREAM_ID = value
        logger.debug(f"Write STREAM_ID: {value}")
    elif index == 2:
        ARGUMENT = value
        logger.debug(f"Write ARGUMENT: {value}")
    elif index == 3:
        FLAGS_SET = value
        logger.debug(f"Write FLAGS_SET: {value}")
    elif index == 4:
        FLAGS_CLR = value
        logger.debug(f"Write FLAGS_CLR: {value}")
    else:
        logger.debug(f"User register {index} not used")

    return value


## @}
