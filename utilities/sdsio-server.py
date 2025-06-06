# Copyright (c) 2023-2025 Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the License); you may
# not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an AS IS BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
from ctypes import wintypes


# ---------------------------------------------------------------------------- #
#           Byte oriented in memory buffer with per-stream flow control        #
# ---------------------------------------------------------------------------- #
class ByteStreamBuffer:
    def __init__(self, max_size=None):
        if max_size is None:
            max_size = 10 * 1024 * 1024  # default to 10 MB
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
    def __init__(self, level=logging.INFO, formatter=None):
        if isinstance(formatter, str):
            formatter = logging.Formatter(formatter)
        self._lock = threading.Lock()
        self._logger = logging.getLogger("sdsio_logger")
        self._progress_dots_num = 0
        self._progress_dots_printing = False

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

    def progress_dots(self, mgr):
        with self._lock:
            if mgr.opened_streams:
                self._progress_dots_printing = True
                if self._progress_dots_num == 16:
                    self._progress_dots_num = 0
                    sys.stdout.write("\r" + " " * 80 + "\r.")
                    sys.stdout.flush()
                else:
                    sys.stdout.write(".")
                sys.stdout.flush()
                self._progress_dots_num += 1

    def _clear_progress(self):
        if self._progress_dots_printing:
            sys.stdout.write("\n")
            sys.stdout.flush()
            self._progress_dots_num = 0
            self._progress_dots_printing = False


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
                printer.progress_dots(self.mgr)
            time.sleep(self.interval)

    def stop(self):
        self._stop.set()
        self._thread.join()


# ---------------------------------------------------------------------------- #
#                            SDS IO Manager                                    #
# ---------------------------------------------------------------------------- #
class sdsio_manager:
    def __init__(self, work_dir):
        self.stream_id = 0
        self.work_dir = path.normpath(work_dir)
        self.opened_streams = {}    # sid -> (file_obj, name, mode)
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

    def _file_write_worker(self, sid, file_obj, buf: ByteStreamBuffer, stop_evt):
        chunk_size = 64 * 1024
        try:
            while True:
                data = buf.read(chunk_size, timeout=0.1)
                if data:
                    file_obj.write(data)
                    continue
                # on EOF, drain any remaining data then exit
                if buf.eof:
                    while True:
                        data = buf.read(chunk_size, timeout=0)
                        if not data:
                            break
                        file_obj.write(data)
                    break
        except Exception as e:
            printer.exception(f"Writer {sid} error")
        finally:
            file_obj.close()

    def _file_read_worker(self, sid, file_obj, buf: ByteStreamBuffer, stop_evt):
        chunk_size = 128 * 1024
        try:
            while not stop_evt.is_set():
                data = file_obj.read(chunk_size)
                if data:
                    buf.write(data)
                else:
                    buf.set_eof()
                    break
        except Exception as e:
            printer.exception(f"Reader {sid} error")
        finally:
            file_obj.close()

    def __open(self, mode, name):
        cmd = 1
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
        for (_, n, _) in self.opened_streams.values():
            if n == name:
                file_name = os.path.basename(self.opened_streams[sid][0].name)
                printer.info(f"Stream '{file_name}' is already opened, cannot open again.")
                return resp_err

        # mode 1 = write, 0 = read
        if mode == 1:
            # write mode: find next filename and start writer thread
            idx = 0
            fname = path.join(self.work_dir, f"{name}.{idx}.sds")
            while path.exists(fname):
                idx += 1
                fname = path.join(self.work_dir, f"{name}.{idx}.sds")
            try:
                f = open(fname, "wb")  # Attempt to open the file in write mode
            except:
                printer.error(f"Failed to open file '{fname}'")
                return resp_err  # Return an error response if the file cannot be opened

            # allocate new sid
            with self.manager_lock:
                self.stream_id += 1
                sid = self.stream_id
            self.opened_streams[sid] = (f, name, mode)
            buf = ByteStreamBuffer(max_size=100 * 1024 * 1024)  # 100 MB
            stop_evt = threading.Event()
            thr = threading.Thread(
                target=self._file_write_worker,
                args=(sid, f, buf, stop_evt),
                daemon=True
            )
            thr.start()
            self.write_buffers[sid] = buf
            self.write_threads[sid] = thr
            self.write_stop[sid]   = stop_evt

        else:
            # read mode: determine file, update index, start reader thread
            idx = 0
            sid = 0
            index_file = path.join(self.work_dir, f"{name}.index.txt")
            if path.exists(index_file):
                try:
                    with open(index_file, "r") as ix:
                        line = ix.readline().strip()
                        if line.isdigit():
                            idx = int(line)
                except:
                    pass
            fname = path.join(self.work_dir, f"{name}.{idx}.sds")

            if  path.exists(fname):
                try:
                    f = open(fname, "rb")  # Attempt to open the file in read mode
                except:
                    printer.error(f"Failed to open file '{fname}'")
                    return resp_err  # Return an error response if the file cannot be opened

                # allocate new sid
                with self.manager_lock:
                    self.stream_id += 1
                    sid = self.stream_id
                self.opened_streams[sid] = (f, name, mode)

                buf = ByteStreamBuffer()
                stop_evt = threading.Event()
                thr = threading.Thread(
                    target=self._file_read_worker,
                    args=(sid, f, buf, stop_evt),
                    daemon=True
                )
                thr.start()
                self.read_buffers[sid] = buf
                self.read_threads[sid] = thr
                self.read_stop[sid]  = stop_evt

            # update index for next read
            try:
                with open(index_file, 'w') as ix:
                    ix.write(str(idx + 1 if path.exists(fname) else idx))
            except Exception:
                printer.warning(f"Could not update index file for {name}")

        # build success response
        resp = bytearray()
        resp.extend(cmd.to_bytes(4,'little'))
        resp.extend(sid.to_bytes(4,'little'))
        resp.extend(mode.to_bytes(4,'little'))
        resp.extend((0).to_bytes(4,'little'))

        file_name = os.path.basename(self.opened_streams[sid][0].name)
        printer.info(f"Stream opened: {self.opened_streams[sid][1]} ({file_name}).")
        return resp

    def __close(self, sid):
        resp = bytearray()
        name = self.opened_streams[sid][1]
        file_name = os.path.basename(self.opened_streams[sid][0].name)
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
        printer.info(f"Stream closed: {name} ({file_name}).")
        return resp

    def __write(self, sid, data):
        resp = bytearray()
        buf = self.write_buffers.get(sid)
        if not buf:
            printer.info(f"Not opened for write: {sid}")
            return resp
        buf.write(data)

        self.time_last_rw = time.time()
        return resp

    def __read(self, sid, size):
        resp = bytearray()
        cmd = 4
        eof = 0
        data = bytearray()
        entry = self.opened_streams.get(sid)
        # invalid read
        if not entry or entry[2] != 0:
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

    def __pingServer(self, sid):
        resp = bytearray()
        cmd = 5
        resp.extend(cmd.to_bytes(4,'little'))
        resp.extend(sid.to_bytes(4,'little'))
        resp.extend((1).to_bytes(4,'little'))
        resp.extend((0).to_bytes(4,'little'))
        printer.info("Ping received.")
        return resp

    def clean(self):
        # close all open streams
        for sid in list(self.opened_streams.keys()):
            self.__close(sid)

    def execute_request(self, buf: bytes):
        cmd = int.from_bytes(buf[0:4],'little')
        sid = int.from_bytes(buf[4:8],'little')
        arg = int.from_bytes(buf[8:12],'little')
        sz  = int.from_bytes(buf[12:16],'little')
        data= buf[16:16+sz]
        if   cmd == 1: return self.__open(arg, data.decode('utf-8').rstrip('\0'))
        elif cmd == 2: return self.__close(sid)
        elif cmd == 3: return self.__write(sid, data)
        elif cmd == 4: return self.__read(sid, arg)
        elif cmd == 5: return self.__pingServer(sid)
        else:
            printer.info(f"Unknown command: {cmd}")
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
        try:
            while True:
                # read fixed-size header, then payload
                hdr = await reader.readexactly(16)
                sz  = int.from_bytes(hdr[12:16],'little')
                pl  = await reader.readexactly(sz) if sz > 0 else b''
                resp= self.manager.execute_request(hdr + pl)
                if resp:
                    writer.write(resp)
                    await writer.drain()
        except (asyncio.IncompleteReadError, ConnectionResetError, OSError):
            printer.info(f"Client disconnected.")
        except Exception:
            printer.info(f"Server Error")
        finally:
            await self._safe_close(reader, writer)
            # Clean up any streams
            self.manager.clean()

    async def start(self):
        self.server = await asyncio.start_server(self.handle_client, self.ip, self.port)
        addr = self.server.sockets[0].getsockname()
        printer.info(f"Socket server listening on {addr[0]}:{addr[1]}")
        async with self.server:
            await self.server.serve_forever()

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
                data = self.read(16 * 1024)
                if data:
                    buffer.extend(data)
                else:
                    time.sleep(0.001)
                    continue

                # Process complete messages from the buffer...
                while len(buffer) >= 16:
                    header = buffer[:16]
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

    def open(self):
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
                    time.sleep(0.5)

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

        except KeyboardInterrupt:
            # let CTRL+C propagate if it happens here
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

    def _on_in_complete(self, xfer: usb1.USBTransfer):
        if xfer.getStatus() == usb1.TRANSFER_COMPLETED:
            data = bytes(xfer.getBuffer()[:xfer.getActualLength()])
            self.loop.call_soon_threadsafe(self.in_q.put_nowait, data)
        if self.running:
            try:
                xfer.submit()
            except usb1.USBError:
                pass

    def _on_out_complete(self, xfer: usb1.USBTransfer):
        self.out_in_flight.discard(xfer)
        self.out_pool.append(xfer)

    def _on_hotplug(self, context, device, event):
        if event & usb1.HOTPLUG_EVENT_DEVICE_LEFT:
            printer.info("USB device disconnected.")
            self.running = False
            # wake the coros so they exit promptly
            self.loop.call_soon_threadsafe(self.in_q.put_nowait,  b'')
            self.loop.call_soon_threadsafe(self.out_q.put_nowait, b'')

    def _monitor_loop(self):
        while self.running:
            time.sleep(0.5)
            # printer.debug("Check device.")
            found = False
            for dev in self.ctx.getDeviceList(skip_on_error=True):
                try:
                    if (dev.getVendorID()  == self.vendor_id and
                        dev.getProductID() == self.product_id):
                        found = True
                        break
                except usb1.USBError:
                    continue
            if not found:
                printer.info("USB device disconnected.")
                self.running = False
                self.loop.call_soon_threadsafe(self.in_q.put_nowait,  b'')
                self.loop.call_soon_threadsafe(self.out_q.put_nowait, b'')
                break

    async def _consumer(self):
        while self.running:
            data = await self.in_q.get()
            self._rx_buf.extend(data)
            while len(self._rx_buf) >= 16:
                hdr   = self._rx_buf[:16]
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

            self.open()

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

            printer.info(f"USB Server running.")

            # start polling thread
            self._poll_thread = threading.Thread(
                target=self._libusb_loop, name="usb-poll", daemon=True
            )
            self._poll_thread.start()

            # run until disconnect or until close() flips self.running=False
            try:
                await asyncio.gather(self._consumer(), self._out_sender())
            except asyncio.CancelledError:
                break

            # clean up and loop back to reconnect
            self.mgr.clean()
            self.close()

    def close(self):
        # Called either by KeyboardInterrupt or by internal errors/disconnects
        self.running = False

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

        if self._poll_thread:
            self._poll_thread.join(timeout=0.2)
        if self._monitor_thread:
            self._monitor_thread.join(timeout=0.2)

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
        raise argparse.ArgumentTypeError(f"Invalid output directory: {work_dir}!")

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
    formatter = lambda prog: argparse.HelpFormatter(prog, max_help_position=41)
    parser = argparse.ArgumentParser(formatter_class=formatter, description="SDS I/O server")
    parser.add_argument("--verbose", "-v", action="store_true", help="Enable debug logging")
    subparsers = parser.add_subparsers(dest="server_type", required=True)

    # Socket server arguments.
    parser_socket = subparsers.add_parser("socket", formatter_class=formatter)
    socket_group = parser_socket.add_argument_group("optional")
    socket_group_exclusive = socket_group.add_mutually_exclusive_group()
    socket_group_exclusive.add_argument("--ipaddr", dest="ip", metavar="<IP>",
                                        help="Server IP address (cannot be used with --interface)",
                                        type=ip_validator, default=None)
    socket_group_exclusive.add_argument("--interface", dest="interface", metavar="<Interface>",
                                        help="Network interface (cannot be used with --ipaddr)",
                                        type=interface_validator, default=None)
    socket_group.add_argument("--port", dest="port", metavar="<TCP Port>",
                              help="TCP port (default: 5050)",
                              type=int, default=5050)
    socket_group.add_argument("--workdir", dest="work_dir", metavar="<Work dir>",
                             help="Directory for SDS files (default: current directory) ", type=dir_path, default=".")

    # Serial server arguments.
    parser_serial = subparsers.add_parser("serial", formatter_class=formatter)
    serial_required = parser_serial.add_argument_group("required")
    serial_required.add_argument("-p", dest="port", metavar="<Serial Port>",
                                 help="Serial port", required=True)
    serial_optional = parser_serial.add_argument_group("optional")
    serial_optional.add_argument("--baudrate", dest="baudrate", metavar="<Baudrate>",
                                 help="Baudrate (default: 115200)", type=int, default=115200)
    parity_help = "Parity: " + ", ".join([f"{k}={v}" for k, v in serial.PARITY_NAMES.items()])
    parity_help += f" (default: {serial.PARITY_NONE})"
    serial_optional.add_argument("--parity", dest="parity", metavar="<Parity>",
                                 choices=serial.PARITY_NAMES.keys(),
                                 help=parity_help, default=serial.PARITY_NONE)
    stopbits_help = (f"Stop bits: {serial.STOPBITS_ONE}, {serial.STOPBITS_ONE_POINT_FIVE}, "
                     f"{serial.STOPBITS_TWO} (default: {serial.STOPBITS_ONE})")
    serial_optional.add_argument("--stopbits", dest="stop_bits", metavar="<Stop bits>",
                                 type=float, choices=[serial.STOPBITS_ONE, serial.STOPBITS_ONE_POINT_FIVE,
                                 serial.STOPBITS_TWO], help=stopbits_help, default=serial.STOPBITS_ONE)
    serial_optional.add_argument("--connect-timeout", dest="connect_timeout", metavar="<Timeout>",
                                 help="Serial port connection timeout in seconds (default: no timeout)",
                                 type=float, default=None)
    serial_optional.add_argument("--workdir", dest="work_dir", metavar="<Work dir>",
                                 help="Directory for SDS files (default: current directory) ", type=dir_path, default=".")

    # USB server arguments.
    parser_usb = subparsers.add_parser("usb", formatter_class=formatter)
    usb_optional = parser_usb.add_argument_group("optional")
    usb_optional.add_argument("--workdir", dest="work_dir", metavar="<Work dir>",
                              help="Directory for SDS files (default: current directory) ", type=dir_path, default=".")
    usb_optional.add_argument("--high-priority", dest="high_priority", action="store_true",
                              help="Enable high-priority threading for USB Server (default: off)")

    return parser.parse_args()

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

    manager = sdsio_manager(args.work_dir)

    try:
        if args.server_type == "socket":
            ip = args.ip
            if not ip and args.interface:
                adapters = ifaddr.get_adapters()
                for adapter in adapters:
                    if adapter.name == args.interface or adapter.nice_name == args.interface:
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
            await sdsio_server_socket_run_supervised(ip, args.port, manager)

        elif args.server_type == "serial":
            sdsio_server_serial_run_supervised(args.port, args.baudrate, args.parity,
                                               args.stop_bits, args.connect_timeout, manager)

        elif args.server_type == "usb":
            loop = asyncio.get_running_loop()
            srv = sdsio_server_usb(manager, loop, high_priority=args.high_priority)

            try:
                printer.info("Starting USB Server...")
                await srv.start()

            except KeyboardInterrupt:
                # This block runs if a KeyboardInterrupt is raised during srv.start()
                printer.info("KeyboardInterrupt received, shutting down.")

            finally:
                # Always attempt to close the USB Server, whether we're here due to Ctrl+C
                # or because srv.start() returned on its own.
                printer.info("Closing USB Server...")
                srv.close()
                # Give background tasks (consumer, out_sender, monitor) a moment to exit.
                await asyncio.sleep(0.1)

    except KeyboardInterrupt:
        pass

    finally:
        # Clean up all SDS streams on exit
        manager.clean()
        printer.info("Server stopped.")

if __name__ == "__main__":
    # minimal printer until main() configures it
    printer = safe_print()
    printer.info("Press Ctrl+C to exit.")
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        printer.info("KeyboardInterrupt received, shutting down.")
