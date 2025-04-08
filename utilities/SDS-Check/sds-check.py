# Copyright (c) 2023 Arm Limited. All rights reserved.
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

# SDS check


import sys
import struct
import argparse
import os.path

info = {"": 0}

class RecordManager:
    def __init__(self):
        self.HEADER_SIZE    = 8
        self.TIMESTAMP_SIZE = 4
        self.data_buff      = bytearray()
        self.timestamp      = []
        self.data_size      = []

    # Flush data buffer
    def __flush(self):
        self.data_buff      = bytearray()
        self.timestamp      = []
        self.data_size      = []

    # Private function for retrieving data from record
    def __getRecord(self, file):
        record = bytearray(file.read(self.HEADER_SIZE))
        if len(record) == self.HEADER_SIZE:
            # Extract timestamp
            self.timestamp.append(struct.unpack("I", record[:self.TIMESTAMP_SIZE])[0])

            # Extract data size
            data_size = struct.unpack("I", record[self.TIMESTAMP_SIZE:])[0]
            self.data_size.append(data_size)

            # Read the data based on the size
            data = bytearray(file.read(data_size))
            # Add the data to the buffer
            self.data_buff.extend(data)
        else:
            # Header is incomplete, error or EOF
            return False
        return True

    # Extract all data from .sds recording and return a dictionary
    # Dictionary consists of: timestamp, data_size, raw_data
    def getData(self, file):
        # Reset internal state before starting to read data
        self.__flush()

        while True:
            # Attempt to read a record
            if not self.__getRecord(file):
                break

        # Return the collected data in a dictionary
        data = {"timestamp" : self.timestamp, \
                "data_size" : self.data_size, \
                "raw_data"  : self.data_buff}

        # After extracting all data, flush internal buffers
        self.__flush()

        return data

# check sizes
def checkSizes(filename,data):
    size = 0
    for n in range(len(data["data_size"])):
        size += 8 + data["data_size"][n]

    file_size = os.path.getsize(filename)

    info["filesize"] = file_size
    info["datasize"] = size

    if file_size != size:
        return False

    return True

# check timestamps
def checkTimestamps(data):
    data_len = len(data["timestamp"])
    if data_len < 2: return True

    # Check if timestamps are in ascending order
    for n in range(1,data_len):
        if data["timestamp"][n] < data["timestamp"][n-1]:
            info["record_id"] = n
            return False

    #Check jitter, save index where jitter is max
    interval = (data["timestamp"][-1] - data["timestamp"][0]) / (data_len-1)

    jitter = index = int(0)
    timestamp = int(data["timestamp"][0] + interval)
    for n in range(1,data_len):
        diff = round(abs(data["timestamp"][n] - timestamp))
        if diff > jitter:
            jitter = diff
            index  = n
        timestamp += interval

    info["records"] = data_len
    info["interval"]= round(interval)
    info["jitter"]  = jitter
    info["index"]   = index

    return True

# main
def main():
    formatter = lambda prog: argparse.HelpFormatter(prog,max_help_position=60)
    parser = argparse.ArgumentParser(description="SDS data validation",
                                     formatter_class=formatter)

    required = parser.add_argument_group("required")
    required.add_argument("-s", dest="sds", metavar="<sds_file>",
                            help="SDS data recording file", nargs="?", required=True)

    args = parser.parse_args()
    filename = args.sds

    # Check if the file exists
    if not os.path.isfile(filename):
        print(f"Error: The file '{filename}' does not exist.")
        sys.exit(1)

    # Load data from SDS file
    Record = RecordManager()

    file = open(filename, "rb")
    data = Record.getData(file)
    file.close()

    if checkSizes(filename,data) == False:
        print(f"Error: File size mismatch. Expected {info["datasize"]} bytes, but file contains {info["filesize"]} bytes.")
        sys.exit(1)

    if checkTimestamps(data) == False:
        print(f"Error: Timestamp not in ascending order in record {info["record_id"]}.")
        sys.exit(1)

    if info["jitter"] == 0: txt_add = ""
    else:                   txt_add = ", record " + str(info["index"])

    print(f"File    : {filename}")
    print(f"Size    : {info["filesize"]:,} bytes")
    print(f"Records : {info["records"]}")
    print(f"Interval: {info["interval"]}ms")
    print(f"Jitter  : {info["jitter"]}ms" + txt_add)

    print("Validation passed")

# main
if __name__ == '__main__':
    main()
