# Copyright (c) 2022-2026 Arm Limited. All rights reserved.
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

# SDS-View

import argparse
import struct
import sys

import matplotlib.pyplot as plt
import numpy as np
import yaml

SDS_VIEW_VERSION = "3.1.2"

class RecordManager:
    def __init__(self):
        self.HEADER_SIZE    = 8
        self.TIMESLOT_SIZE  = 4
        self.data = bytearray()
        self.timeslot = []
        self.data_size = []

    # Flush data buffer
    def flush(self):
        self.data = bytearray()
        self.timeslot = []
        self.data_size = []

    # Private function for retrieving data from record
    def __getRecord(self, file):
        record = bytearray(file.read(self.HEADER_SIZE))
        if len(record) == self.HEADER_SIZE:
            timeslot  = struct.unpack("I", record[:self.TIMESLOT_SIZE])[0]
            data_size = struct.unpack("I", record[self.TIMESLOT_SIZE:])[0]
            self.timeslot.append(timeslot)
            self.data_size.append(data_size)
            self.data.extend(bytearray(file.read(data_size)))
            return True
        else:
            return False

    # Extract all data from recording file
    def getData(self, file):
        record = True
        while record:
            record = self.__getRecord(file)
        return {"raw_data" : self.data, "timeslot" : self.timeslot, "data_size" : self.data_size}


def getSampleFrequency(meta_data):
    if "sample-frequency" in meta_data:
        return meta_data["sample-frequency"]
    if "frequency" in meta_data:
        return meta_data["frequency"]
    return None


def getPlotFrequency(meta_data, data):
    sample_frequency = getSampleFrequency(meta_data)
    if sample_frequency is not None:
        if not sample_frequency > 0:
            print(f"Error: Sample frequency must be greater than 0 (f = {sample_frequency})\n")
            sys.exit(0)
        return sample_frequency

    timeslot = data["timeslot"]
    if len(timeslot) < 2:
        return None

    delta = timeslot[1] - timeslot[0]
    if delta <= 0:
        return None

    tick_frequency = meta_data.get("tick-frequency", 1000)
    if not tick_frequency > 0:
        print(f"Error: Tick frequency must be greater than 0 (f = {tick_frequency})\n")
        sys.exit(0)

    return tick_frequency / delta


def checkSupportedMetadata(meta_data):
    for entry in meta_data:
        if not (("value" in entry and "type" in entry) or
                (isinstance(entry.get("audio"), dict) and entry["audio"].get("bit-depth") in (8, 16, 32) and entry["audio"].get("channels") == 1 and entry["audio"].get("format") == "pcm")):
            if "value" in entry and not "type" in entry:
                sys.exit("Error: SDS-View supports content `value:` only with `type:`")
            elif isinstance(entry.get("audio"), dict):
                sys.exit("Error: SDS-View supports content `audio:` only with `bit-depth: 8, 16 or 32`, `channels: 1` and `format: pcm`")
            else:
                sys.exit("Error: Unknown or unsupported content, supported contents are only `value` and `audio`")

# Convert C style data type to Python style
def getDataType(data_type):
    if   data_type == "int8_t" or data_type == 8:
        d_type = "b"
    elif data_type == "uint8_t":
        d_type = "B"
    elif data_type == "int16_t" or data_type == 16:
        d_type = "h"
    elif data_type == "uint16_t":
        d_type = "H"
    elif data_type == "int32_t" or data_type == 32:
        d_type = "i"
    elif data_type == "uint32_t":
        d_type = "I"
    elif data_type == "float":
        d_type = "f"
    elif data_type == "double":
        d_type = "d"
    else:
        print(f"Unknown or unsupported data type: {data_type}\n")
        d_type = "I"

    return d_type

# Open SDS file in read mode
def openFile(file_name):
    try:
        if ".yml" in file_name:
            file = open(file_name, "r")
        else:
            file = open(file_name, "rb")
        return file
    except Exception as e:
        print(f"Error in openFile({file_name}): {e}")
        sys.exit(1)

# Close file
def closeFile(file_name):
    try:
        file_name.close()
    except Exception as e:
        print(f"Error in closeFile({file_name}): {e}")
        sys.exit(1)

# Create new figure and plot content
def plotData(all_data, data_desc, freq, tick_frequency, title, view3D):
    raw_data = all_data["raw_data"]
    dim = {}
    desc_n = 0
    desc_n_max = len(data_desc)

    # Create a new figure for each .sds file
    fig = plt.figure()
    for desc in data_desc:
        # Extract parameters from description in YAML file
        if "unit" in desc:
            unit = desc["unit"]
        else:
            unit = "raw"

        if "scale" in desc:
            scale = desc["scale"]
        else:
            scale = 1

        if "offset" in desc:
            offset = desc["offset"]
        else:
            offset = 0

        if "type" in desc:
            d_type = getDataType(desc["type"])
        elif "audio" in desc:
            d_type = getDataType(desc["audio"]["bit-depth"])
        else:
            sys.exit(1)

        # Calculate number of bytes needed for decoding the data in .sds file
        d_byte = struct.calcsize(d_type)
        # Disunite raw data into a list of data points according to the number of bytes needed for each data point
        tmp_data = [raw_data[i:(i + d_byte)] for i in range(0, len(raw_data), d_byte)]
        # Keep only every n-th data point
        tmp_data = tmp_data[desc_n::desc_n_max]
        # Decode retrieved data points
        data = struct.unpack(f"{int(len(tmp_data))}{d_type}", b''.join(tmp_data))
        # Scale and offset data points
        scaled_data = [((x * scale) + offset) for x in data]

        # Keep each record at its original timeslot and break the line at empty records.
        t = []
        plot_data = []
        index = 0
        for timeslot, data_size in zip(all_data["timeslot"], all_data["data_size"]):
            record_time = (timeslot - all_data["timeslot"][0]) / tick_frequency
            count = data_size // (d_byte * desc_n_max)
            if count == 0:
                t.append(record_time)
                plot_data.append(np.nan)
            else:
                t.extend(record_time + (np.arange(count) / freq if freq else np.zeros(count)))
                plot_data.extend(scaled_data[index:index + count])
                index += count
        if "type" in desc:
            plt.plot(t, plot_data, label=desc["value"])
        elif "audio" in desc:
            plt.plot(t, plot_data, label="Audio")
        else:
            sys.exit(1)

        # Store data points in a dictionary for later use when there are 3 axes described
        if view3D and (desc_n_max == 3):
            dim[desc_n] = plot_data

        # Increment description number
        desc_n += 1

    if all_data["timeslot"]:
        timeslots = all_data["timeslot"]
        interval = timeslots[-1] - timeslots[-2] if len(timeslots) > 1 else 0
        plt.xlim(0, (timeslots[-1] - timeslots[0] + interval) / tick_frequency)
    plt.grid(linestyle=":")
    plt.title(title)
    plt.xlabel("seconds")
    plt.ylabel(unit)
    plt.legend()

    # Create a 3D view when there are 3 axes available
    if view3D and (desc_n_max == 3):
        fig3d = plt.figure()
        ax3d = fig3d.add_subplot(projection="3d")
        ax3d.plot(dim[0], dim[1], dim[2])
        ax3d.set_title(f"{title} - 3D")
        ax3d.set_xlabel(f"{data_desc[0]['value']} [{data_desc[0]['unit']}]")
        ax3d.set_ylabel(f"{data_desc[1]['value']} [{data_desc[1]['unit']}]")
        ax3d.set_zlabel(f"{data_desc[2]['value']} [{data_desc[2]['unit']}]")

# Main function
def main():
    formatter = lambda prog: argparse.HelpFormatter(prog,max_help_position=60)
    parser = argparse.ArgumentParser(description="View SDS data",
                                     formatter_class=formatter,
                                     add_help=False)
    parser.add_argument("-h", "--help", action="help",
                          help="Show this help message and exit")
    parser.add_argument("-V", "--version", action="version",
                          help="Show program's version number and exit",
                          version=f"SDS-View {SDS_VIEW_VERSION}")

    required = parser.add_argument_group("required")
    required.add_argument("-i", dest="sds", metavar="<sds_file>",
                            help="SDS files", nargs="+", required=True)
    required.add_argument("-y", dest="yaml", metavar="<yaml_file>",
                            help="YAML metadata file", required=True)

    optional = parser.add_argument_group("optional")
    optional.add_argument("--3D", dest="view3D",
                            help="Plot 3D view in addition to normal 2D", action="store_true")

    # Parse arguments - show help if no arguments provided
    if len(sys.argv) == 1:
        parser.print_help(sys.stderr)
        sys.exit(1)

    args = parser.parse_args()

    # Load data from .yml file
    meta_file = openFile(args.yaml)
    meta_data = yaml.load(meta_file, Loader=yaml.FullLoader)["sds"]
    closeFile(meta_file)

    # Parse description file
    data_name = meta_data["name"]
    data_desc = meta_data["content"]
    if not data_desc:
        sys.exit("Error: SDS metadata content must not be empty")
    checkSupportedMetadata(data_desc)

    # Record manager
    Record = RecordManager()

    # Read .sds file/files
    for arg in args.sds:
        file = openFile(arg)
        data = Record.getData(file)
        closeFile(file)
        Record.flush()
        data_freq = getPlotFrequency(meta_data, data)

        # Plot data from .sds file/files
        plotData(data, data_desc, data_freq, meta_data.get("tick-frequency", 1000), data_name, args.view3D)

    # Show plotted figures
    plt.show()


if __name__ == "__main__":
    main()
