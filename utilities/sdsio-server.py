# Copyright (c) 2023-2025 Arm Limited. All rights reserved.
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
# distributed under the License is distributed on an AS IS BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import argparse
import sys
import os.path as path
import serial
import ipaddress
import ifaddr
import socket
import threading
import time
import asyncio

# ---------------------------------------------------------------------------- #
#           Byte oriented in memory buffer with per-stream flow control        #
# ---------------------------------------------------------------------------- #
class ByteStreamBuffer:
    def __init__(self, max_size=None):
        self.buf = bytearray()
        self.lock = threading.Lock()
        self.data_avail = threading.Condition(self.lock)
        self.eof = False
        self.max_size = max_size

    def write(self, data: bytes):
        # Block if buffer full (when max_size set), then append data
        with self.data_avail:
            if self.max_size:
                while len(self.buf) + len(data) > self.max_size:
                    self.data_avail.wait()
            self.buf.extend(data)
            self.data_avail.notify_all()

    def read(self, amt: int, timeout=None) -> bytes:
        # Wait for data or EOF, then return up to 'amt' bytes
        with self.data_avail:
            if not self.buf and not self.eof:
                self.data_avail.wait(timeout)
            if not self.buf:
                return b''
            chunk = self.buf[:amt]
            del self.buf[:amt]
            self.data_avail.notify_all()
            return bytes(chunk)

    def set_eof(self):
        # Signal end-of-file and wake any waiting readers
        with self.data_avail:
            self.eof = True
            self.data_avail.notify_all()

# ---------------------------------------------------------------------------- #
#                                 SDS IO Manager                                #
# ---------------------------------------------------------------------------- #
class sdsio_manager:
    def __init__(self, out_dir):
        self.stream_id = 0
        self.out_dir = path.normpath(out_dir)
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
            print(f"[Writer {sid}] error: {e}")
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
            print(f"[Reader {sid}] error: {e}")
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
            print(f"Invalid stream name: {name}")
            return resp_err
        invalid_chars = [chr(i) for i in range(0x00, 0x10)] + [chr(0x7F), '"', '*', '/', ':', '<', '>', '?', '\\', '|']
        if any(ch in name for ch in invalid_chars):
            print(f"Invalid stream name: {name}")
            return resp_err

        # ensure not already open
        for (_, n, _) in self.opened_streams.values():
            if n == name:
                print(f"Stream '{name}' is already opened, cannot open again.")
                return resp_err

        # allocate new sid
        with self.manager_lock:
            self.stream_id += 1
            sid = self.stream_id

        # mode 1 = write, 0 = read
        if mode == 1:
            # write mode: find next filename and start writer thread
            idx = 0
            fname = path.join(self.out_dir, f"{name}.{idx}.sds")
            while path.exists(fname):
                idx += 1
                fname = path.join(self.out_dir, f"{name}.{idx}.sds")
            f = open(fname, "wb")
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
            index_file = path.join(self.out_dir, f"{name}.index.txt")
            if path.exists(index_file):
                with open(index_file, "r") as ix:
                    line = ix.readline().strip()
                    if line.isdigit():
                        idx = int(line)
            fname = path.join(self.out_dir, f"{name}.{idx}.sds")
            if not path.exists(fname) and idx != 0:
                idx = 0
                fname = path.join(self.out_dir, f"{name}.{idx}.sds")
            with open(index_file, "w") as ix:
                ix.write(str(idx + 1 if path.exists(fname) else idx))
            f = open(fname, "rb")
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

        # build success response
        resp = bytearray()
        resp.extend(cmd.to_bytes(4,'little'))
        resp.extend(sid.to_bytes(4,'little'))
        resp.extend(mode.to_bytes(4,'little'))
        resp.extend((0).to_bytes(4,'little'))
        print(f"Stream opened: '{self.opened_streams[sid][1]}'.")
        return resp

    def __close(self, sid):
        resp = bytearray()
        name = self.opened_streams[sid][1]
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
        print(f"Stream closed: {name}.")
        return resp

    def __write(self, sid, data):
        resp = bytearray()
        buf = self.write_buffers.get(sid)
        if not buf:
            print(f"Not opened for write: {sid}")
            return resp
        buf.write(data)
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
        return resp

    def __pingServer(self, sid):
        resp = bytearray()
        cmd = 5
        resp.extend(cmd.to_bytes(4,'little'))
        resp.extend(sid.to_bytes(4,'little'))
        resp.extend((1).to_bytes(4,'little'))
        resp.extend((0).to_bytes(4,'little'))
        print("Ping received. Connection is active.")
        return resp

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
            print(f"Unknown command: {cmd}")
            return bytearray()

# ---------------------------------------------------------------------------- #
#                            Async Socket Server                               #
# ---------------------------------------------------------------------------- #
class async_sdsio_server_socket:
    def __init__(self, ip, port, manager: sdsio_manager):
        self.ip = ip
        self.port = port
        self.manager = manager

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
        except (asyncio.IncompleteReadError, ConnectionResetError, OSError) as e:
            print(f"Client disconnected: {e}")
        except Exception as e:
            print(f"Error in handle_client: {e}")
        finally:
            writer.close()
            await writer.wait_closed()

    async def start(self):
        server = await asyncio.start_server(self.handle_client, self.ip, self.port)
        addr = server.sockets[0].getsockname()
        print(f"Socket server listening on {addr}")
        async with server:
            await server.serve_forever()

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
        print(f"  Serial Port: {self.port}")
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
        except Exception as e:
            print(f"Error initializing serial.Serial: {e}")
            sys.exit(1)
        start_time = time.time()
        while True:
            try:
                self.ser.open()
                print("Serial port opened successfully.")
                break
            except Exception as e:
                if time.time() - start_time >= self.connect_timeout:
                    print(f"Serial port open failed after {self.connect_timeout} seconds. Error: {e}")
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
        except Exception as e:
            print(f"Serial read error: {e}")
            sys.exit(1)

    def write(self, data):
        try:
            return self.ser.write(data)
        except Exception as e:
            print(f"Serial write error: {e}")
            sys.exit(1)

    def start(self):
        self.open()
        print("Serial Server started.")
        buffer = bytearray()
        while True:
            try:
                data = self.read(16 * 1024)
            except Exception as e:
                print(f"Error during blocking read: {e}")
                time.sleep(0.001)
                continue

            if data:
                buffer.extend(data)
            else:
                continue

            # Process complete messages from the buffer.
            # (Assuming that each message has at least a 16-byte header in which bytes [12:16] encode data_size.)
            while len(buffer) >= 16:
                header = buffer[:16]
                data_size = int.from_bytes(header[12:16], 'little')
                req_len = 16 + data_size
                if len(buffer) < req_len:
                    break  # Wait for more data.
                request_buf = buffer[:req_len]
                buffer = buffer[req_len:]
                response = self.manager.execute_request(request_buf)
                if response:
                    self.write(response)

# ---------------------------------------------------------------------------- #
#                        Argument Parsing & Entry Point                        #
# ---------------------------------------------------------------------------- #
def dir_path(out_dir):
    if path.isdir(out_dir):
        return out_dir
    else:
        raise argparse.ArgumentTypeError(f"Invalid output directory: {out_dir}!")

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
    parser = argparse.ArgumentParser(formatter_class=formatter, description="SDS I/O server with async I/O")
    subparsers = parser.add_subparsers(dest="server_type", required=True)

    # Socket server arguments.
    parser_socket = subparsers.add_parser("socket", formatter_class=formatter)
    socket_group = parser_socket.add_argument_group("optional")
    socket_group_exclusive = socket_group.add_mutually_exclusive_group()
    socket_group_exclusive.add_argument("--ipaddr", dest="ip", metavar="<IP>",
                                        help="Server IP address (not allowed with --interface)",
                                        type=ip_validator, default=None)
    socket_group_exclusive.add_argument("--interface", dest="interface", metavar="<Interface>",
                                        help="Network interface (not allowed with --ipaddr)",
                                        type=interface_validator, default=None)
    socket_group.add_argument("--port", dest="port", metavar="<TCP Port>",
                              help="TCP port (default: 5050)",
                              type=int, default=5050)
    socket_group.add_argument("--outdir", dest="out_dir", metavar="<Output dir>",
                              help="Output directory", type=dir_path, default=".")

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
                                 help="Serial port connection timeout in seconds (default: 60)",
                                 type=float, default=60)
    serial_optional.add_argument("--outdir", dest="out_dir", metavar="<Output dir>",
                                 help="Output directory", type=dir_path, default=".")

    return parser.parse_args()

async def main():
    args = parse_arguments()
    mgr = sdsio_manager(args.out_dir)
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
        from asyncio import start_server
        server = async_sdsio_server_socket(ip, args.port, mgr)
        await server.start()
    else:
        srv = sdsio_server_serial(
            args.port, args.baudrate, args.parity,
            args.stop_bits, args.connect_timeout, mgr
        )
        srv.start()

if __name__ == "__main__":
    print("Press Ctrl+C to exit.")
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("KeyboardInterrupt received, shutting down.")
