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

info = {}

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
    data_size = 0
    smallest  = largest = data["data_size"][0]

    # check data size, find the smallest and largest record
    for n in range(len(data["data_size"])):
        if data["data_size"][n] > largest:
            largest = data["data_size"][n]
        elif data["data_size"][n] < smallest:
            smallest = data["data_size"][n]
        data_size += data["data_size"][n]

    info["file_size"] = os.path.getsize(filename)
    info["data_size"] = 8 * len(data["data_size"]) + data_size
    info["block_size"]= round(data_size / len(data["data_size"]))
    info["smallest"]  = smallest
    info["largest"]   = largest

    return info["file_size"] == info["data_size"]

# check timestamps
def checkTimestamps(data):
    data_len = len(data["timestamp"])
    info["records"] = data_len

    if data_len < 2: return True

    # Check if timestamps are in ascending order
    # and find the largest delta time
    info["delta_time"] = int(0)
    for n in range(1,data_len):
        if data["timestamp"][n] < data["timestamp"][n-1]:
            # Error: Not in ascending order
            info["record_id"] = n
            return False
        if data["timestamp"][n] - data["timestamp"][n-1] > info["delta_time"]:
            info["delta_time"]  = data["timestamp"][n] - data["timestamp"][n-1]
            info["delta_index"] = n

    # Check whether timestamps are duplicated
    # and save the index at which sequence begins
    info["dup_count"] = count = int(0)
    for n in range(1,data_len):
        if data["timestamp"][n] == data["timestamp"][n-1]:
            count += 1
            if count > info["dup_count"]:
                if count == 1:
                    info["dup_index"] = n
                info["dup_count"] = count
        else:
            count = 0


    #Check the jitter, save the index at which the jitter is greatest
    interval = (data["timestamp"][-1] - data["timestamp"][0]) / (data_len-1)
    timestamp = data["timestamp"][0] + interval

    info["interval"] = round(interval)
    info["jitter"] = int(0)

    for n in range(1,data_len):
        diff = round(abs(data["timestamp"][n] - timestamp))
        if diff > info["jitter"]:
            info["jitter"] = diff
            info["index"]  = n
        timestamp += interval

    return True

# Convert int value to dot formatted string
def dot(value):
    if value < 1000:    return str(value)
    if value < 1000000: return str(value//1000)    + '.' + str(value%1000).zfill(3)
    else:               return str(value//1000000) + '.' + str(value%1000000//1000).zfill(3) + '.' +  str(value%1000).zfill(3)

# main
def main():
    # Process arguments
    formatter = lambda prog: argparse.HelpFormatter(prog,max_help_position=60)
    parser = argparse.ArgumentParser(description="SDS data validation", formatter_class=formatter)

    required = parser.add_argument_group("required")
    required.add_argument("-s", dest="sds", metavar="<sds_file>", help="SDS data recording file", required=True)

    args = parser.parse_args()
    filename = args.sds

    # Check if the file exists
    if not os.path.isfile(filename):
        print(f"Error: The file '{filename}' does not exist.")
        sys.exit(1)

    # Check if the file is empty
    if os.path.getsize(filename) == 0:
        print(f"Error: The file '{filename}' is empty.")
        sys.exit(1)

    # Load data from SDS file
    Record = RecordManager()

    file = open(filename, "rb")
    data = Record.getData(file)
    file.close()

    if not checkSizes(filename,data):
        print(f"Error: File size mismatch. Expected {dot(info['data_size'])} bytes, but file contains {dot(info['file_size'])} bytes.")
        sys.exit(1)

    if not checkTimestamps(data):
        print(f"Error: Timestamp not in ascending order in record {dot(info['record_id'])}.")
        sys.exit(1)

    # Print summary
    print(f"File     : {filename}")
    print(f"DataSize : {dot(info['file_size'])} bytes")
    print(f"Records  : {dot(info['records'])}")

    print(f"BlockSize: {dot(info['block_size'])} bytes")
    if info["largest"] > info["block_size"] or info["smallest"] < info["block_size"]:
        print(f"Largest  : {dot(info['largest'])} bytes")
        print(f"Smallest : {dot(info['smallest'])} bytes")

    print(f"Interval : {dot(info['interval'])} ms")

    datarate = round((info["block_size"] * 1000) / info["interval"])
    print(f"DataRate : {dot(datarate)} byte/s")

    if info["jitter"] > 0:
        print(f"Jitter   : {dot(info['jitter'])} ms, record {dot(info['index'])}")
    else:
        print("Jitter   : 0 ms")

    if info["delta_time"] > info["interval"]:
        print(f"DeltaTime: {dot(info['delta_time'])} ms, record {dot(info['delta_index'])}")

    if info["dup_count"] > 0:
        print(f"DupStamps: {dot(info['dup_count'])}, record {dot(info['dup_index'])}")

    print("Validation passed")

# main
if __name__ == '__main__':
    main()
