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

# Python SDS Data Converter

import argparse
import csv
import json
import sys
import wave
from struct import calcsize, unpack

import numpy as np
import pandas as pd
import yaml


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
            self.timestamp.append(unpack("I", record[:self.TIMESTAMP_SIZE])[0])
            self.data_size.append(unpack("I", record[self.TIMESTAMP_SIZE:])[0])
            self.data_buff.extend(bytearray(file.read(self.data_size[-1])))

    # Extract all data from .sds recording and return a dictionary
    # Dictionary consists of: timestamp, data_size, raw_data
    def getData(self, file):
        record_num = 0

        while True:
            self.__getRecord(file)
            if len(self.timestamp) == record_num:
                break
            else:
                record_num += 1

        data = {"timestamp" : self.timestamp, \
                "data_size" : self.data_size, \
                "raw_data" : self.data_buff}

        self.__flush()

        return data


# Potential AutoML CSV V2 columns that are not sensors, and so should be ignored.
EXCLUDE_QX_CSVV2_COLS = ('timestamp', 'index', 'event', 'label', 'data_type', 'seconds', 'recording_id', 'event_id')

# Convert C style data type to Python style
def getDataType(data_type):
    if   data_type == "int16_t":
        d_type = "h"
    elif data_type == "uint16_t":
        d_type = "H"
    elif data_type == "int32_t":
        d_type = "i"
    elif data_type == "uint32_t":
        d_type = "I"
    elif data_type == "float":
        d_type = "f"
    elif data_type == "double":
        d_type = "d"
    else:
        print(f"Unknown data type: {data_type}\n")
        d_type = "I"

    return d_type

# Select correct Qeexo sensor data name
def qeexoColumnName(sensor_name):
    if   sensor_name == 'Accelerometer':
        qeexo_name = 'accel'
    elif sensor_name == 'Gyroscope':
        qeexo_name = 'gyro'
    elif sensor_name == 'Magnometer':
        qeexo_name = 'magno'
    elif sensor_name == 'Temperature':
        qeexo_name = 'temperature'
    elif sensor_name == 'Humidity':
        qeexo_name = 'humidity'
    elif sensor_name == 'Pressure':
        qeexo_name = 'pressure'
    elif sensor_name == 'Microphone':
        qeexo_name = 'microphone'
    elif sensor_name == 'Analog microphone':
        qeexo_name = 'microphone_analog'
    elif sensor_name == 'Light':
        qeexo_name = 'light'
    elif sensor_name == 'Ambient light':
        qeexo_name = 'ambient'
    elif sensor_name == 'RCDA':
        qeexo_name = 'rcda'
    elif sensor_name == 'ETOH':
        qeexo_name = 'etoh'
    elif sensor_name == 'TVOC':
        qeexo_name = 'tvoc'
    elif sensor_name == 'IAQ':
        qeexo_name = 'iaq'
    elif sensor_name == 'ECO2':
        qeexo_name = 'eco2'
    elif sensor_name == 'RMOX':
        qeexo_name = 'rmox'
    elif sensor_name == 'Low power accelerometer':
        qeexo_name = 'accel_lowpower'
    elif sensor_name == 'High sensitivity accelerometer':
        qeexo_name = 'accel_highsensitive'
    else:
        qeexo_name = sensor_name.lower()

    return qeexo_name


# Open CSV file and read it's contents
def readCSV(filename):
    global sensors, csv_data

    try:
        csv_data = pd.read_csv(f'{filename}.csv')

        sensors = csv_data.columns.tolist()
        sensors = set(sensors) - set(EXCLUDE_QX_CSVV2_COLS)
    except Exception as e:
        sys.exit(f"Error in readCSV(): {e}")

# Open CSV file and create writer
def createCSV(filename):
    global csv_file, writer

    try:
        csv_file = open(f"{filename}.csv", "w", newline='')
        writer = csv.writer(csv_file)
    except Exception as e:
        sys.exit(f"Error in createCSV(): {e}")

# Open CSV file and create writer
def createWAV(filename):
    global wave_file

    try:
        wave_file = wave.open(f"{filename}.wav", "wb")
    except Exception as e:
        sys.exit(f"Error in createWAV(): {e}")

def prepareData(meta_data, raw_data, data_manipulation):
    sensor_data = []
    desc_n_max = len(meta_data)
    desc_n = 0
    for channel in meta_data:
        tmp_data = []
        # Extract channel data type information from YAML file
        d_type = getDataType(channel["type"])
        # Calculate number of bytes needed for decoding the data in .sds file
        d_byte = calcsize(d_type)
        # Disunite raw data into a list of data points according to the number
        # of bytes needed for each data point
        tmp_data = [raw_data[i:(i + d_byte)] for i in range(0, len(raw_data), d_byte)]
        # Keep only every n-th data point
        tmp_data = tmp_data[desc_n::desc_n_max]
        # Decode retrieved data points
        tmp_data = list(unpack(f"{int(len(tmp_data))}{d_type}", b''.join(tmp_data)))
        # Scale and offset data points if output format is not Qeexo V2 CSV
        if data_manipulation == True:
            if "scale" in channel:
                scale = channel["scale"]
            else:
                scale = 1
            if "offset" in channel:
                offset = channel["offset"]
            else:
                offset = 0
            data = [((x * scale) + offset) for x in tmp_data]
        else:
            data = tmp_data
        # Store decoded data in a dictionary
        sensor_data.append(data)
        # Increment channel description number
        desc_n += 1

    return sensor_data

# Write data to CSV file, simple format
# Only supports one sensor at a time
def write_SDS_SimpleCSV(args, data, meta_data):
    normalize = args.normalize
    csv_start_tick = args.start_tick
    csv_stop_tick = args.stop_tick

    # Automatically generate new column for each sensor channel
    csv_header = ['timestamp']
    for channel in meta_data:
        channel_name = channel["value"]
        csv_header.append(channel_name)

    # Write header to CSV file
    writer.writerow(csv_header)

    # Convert [ms] to [s]
    timestamp = [t / 1000 for t in data["timestamp"]]
    data_size = data["data_size"]

    cnt = 0
    cnt_max = len(timestamp)

    while True:
        # Break while loop if end of file is reached
        if cnt == cnt_max:
            break

        # Create a list of lists, based on the number of channels
        record_row = [[] for i in range(0, len(meta_data))]

        # Extract needed data and flush used bytes from buffer
        raw_data  = data["raw_data"][:data_size[cnt]]
        data["raw_data"] = data["raw_data"][data_size[cnt]:]

        # Convert raw data according to description in meta data
        record_row = prepareData(meta_data, raw_data, data_manipulation=True)

        # Prepare and write CSV row to output file if record_row is not empty
        if record_row != [[] for i in range(0, len(meta_data))]:
            csv_timestamp = timestamp[cnt]
            # Normalize output timestamps
            if normalize == True:
                csv_timestamp -= timestamp[0]
            # Interpolate timestamps if there is more than one data point in this record
            if len(record_row[0]) > 1:
                # Get number of samples in this record
                n_samples = len(record_row[0])

                # Reuse previous time difference to interpolate timestamps for final record
                if cnt == (cnt_max-1):
                    csv_timestamp_next = csv_timestamp + (csv_timestamp - csv_timestamp_previous)
                else:
                    csv_timestamp_next = timestamp[cnt + 1]
                    if normalize == True:
                        csv_timestamp_next -= timestamp[0]

                # Interpolate timestamps between current and next record, based on the number of
                # data points in current record
                tmp_time = np.linspace(csv_timestamp, csv_timestamp_next,  n_samples + 1)[:-1]
                csv_timestamp_previous = csv_timestamp

                # Write generated CSV row to output file
                for t in range(0, len(tmp_time)):
                    # If start/stop tick parameters are specified, write to output file
                    # when timestamps are between selected boundaries
                    if (csv_start_tick == None) or (tmp_time[t] >= csv_start_tick):
                        if (csv_stop_tick == None) or (csv_stop_tick > tmp_time[t]):
                            tmp_record_row = [record_row[i][t] for i in range(len(record_row))]
                            writer.writerow([float(tmp_time[t])] + tmp_record_row)
                        else:
                            break
            else:
                # If start/stop tick parameters are specified, write to output file
                # when timestamps are between selected boundaries
                if (csv_start_tick == None) or (csv_timestamp >= csv_start_tick):
                    if (csv_stop_tick == None) or (csv_stop_tick > csv_timestamp):
                        # Write generated CSV row to output file
                        writer.writerow([float(csv_timestamp)] + record_row[0])
                    else:
                        break
            cnt += 1
        else:
            break


# Write data to CSV file using Qeexo V2 format
def write_SDS_QeexoV2CSV(args, data, meta_data):
    interval = args.interval
    normalize = args.normalize
    csv_start_tick = args.start_tick
    csv_stop_tick = args.stop_tick

    csv_header = ['timestamp']
    for sensor in meta_data:
        sensor_name = qeexoColumnName(sensor)
        csv_header.append(sensor_name)
    csv_header.append('label')

    writer.writerow(csv_header)

    # Set sensor position counters to 0 and extract base timestamps for each sensor
    cnt = {}
    cnt_old = {}
    timestamp_base = []
    for sensor in data:
        cnt_old[sensor] = 0
        cnt[sensor] = 0
        timestamp_base.append(data[sensor]["timestamp"][0])

    # Select timestamp with lowest value and round to first next interval
    csv_timestamp_base = (min(timestamp_base) // interval) * interval
    csv_timestamp = csv_timestamp_base + interval

    while True:
        # Create a list of lists, based on the number of sensors
        csv_row = [[] for i in range(0, len(data.keys()))]
        sensor_idx = 0

        for sensor in data:
            timestamp = data[sensor]["timestamp"]
            data_size = data[sensor]["data_size"]

            # Check if end of file is reached. If it is, skip current sensor
            # Otherwise store current position counter in cnt_old
            if cnt[sensor] < len(timestamp):
                cnt_old[sensor] = cnt[sensor]
            else:
                continue

            # Increment position counter of this sensor if elapsed time in record
            # is less than next CSV timestamp
            while timestamp[cnt[sensor]] < csv_timestamp:
                cnt[sensor] += 1
                # Break while loop if end of file is reached
                if cnt[sensor] == len(timestamp):
                    break

            # Calculate number of data bytes for current timestamp
            n_bytes = sum(data_size[cnt_old[sensor]:cnt[sensor]])

            # Extract needed data and flush used bytes from buffer
            raw_data = data[sensor]["raw_data"][:n_bytes]
            data[sensor]["raw_data"] = data[sensor]["raw_data"][n_bytes:]

            # Convert raw data according to description in meta data
            sensor_data = prepareData(meta_data[sensor], raw_data, data_manipulation=False)

            # Group every n-th element of each channel into a list
            csv_data = []
            if len(sensor_data) > 1:
                for i in range(0, len(sensor_data[0])):
                    tmp_data = []
                    for channel in range(0, len(sensor_data)):
                        tmp_data.append(sensor_data[channel][i])
                    csv_data.append(tmp_data)
            elif len(sensor_data) == 1:
                csv_data = sensor_data[0]

            # Insert sensor data for this CSV timestamp
            csv_row[sensor_idx] = csv_data
            sensor_idx += 1

        # Write current row into CSV file and increment CSV timestamp by one interval.
        # If there is no data present in this row, exit while loop without writing to the file.
        if csv_row != [[] for i in range(0, len(data.keys()))]:
            if normalize == True:
                tmp_csv_timestamp = csv_timestamp - csv_timestamp_base
            else:
                tmp_csv_timestamp = csv_timestamp

            # If start/stop tick parameters are specified, write to output file
            # when timestamps are between selected boundaries
            if (csv_start_tick == None) or (tmp_csv_timestamp >= csv_start_tick):
                if (csv_stop_tick == None) or (csv_stop_tick > tmp_csv_timestamp):
                    writer.writerow([tmp_csv_timestamp] + csv_row + [args.label])
                else:
                    break
            csv_timestamp += interval
        else:
            break

    csv_file.close()


# Write data from QeexoV2 CSV format into SDS files
def write_QeexoV2CSV_SDS(index):
    # Write each sensor's data to it's own SDS file named: <sensor>.<recording_id>.sds
    for sensor in sensors:
        filename = f'{sensor}.{index}.sds'
        with open(filename, 'wb') as f:
            for idx, row in csv_data.iterrows():
                start_ts = int(row['timestamp'])
                # Sensor data is a string which is a JSON array containing all of the samples
                # at this 50ms timestamp block
                all_samples = row[sensor]
                data = json.loads(all_samples)
                if len(data)>0:
                    # We have data to write out.
                    ts_step = 50 / float(len(data))
                    cur_ts = float(start_ts)

                    # Write one record (timestamp, data size, binary data) of selected sensor to SDS file
                    for sample in data:
                        f.write(int(cur_ts).to_bytes(4, byteorder='little', signed=False))

                        if (sensor == 'accel') or (sensor == 'gyro') or (sensor == 'magno'):
                            # These are each 3-axes sensors (x,y,z) with int16 (2 byte) values
                            assert(len(sample) == 3)
                            data_size = int(3 * 2)
                            sample_x = int(sample[0])
                            sample_y = int(sample[1])
                            sample_z = int(sample[2])
                            f.write(data_size.to_bytes(4, byteorder='little', signed=False))
                            f.write(sample_x.to_bytes(2, byteorder='little', signed=True))
                            f.write(sample_y.to_bytes(2, byteorder='little', signed=True))
                            f.write(sample_z.to_bytes(2, byteorder='little', signed=True))
                        elif (sensor == 'temperature') or (sensor == 'humidity') or (sensor == 'microphone'):
                            # These are single axes sensors with int16 (2 byte) values
                            assert(len(sample) == 1)
                            data_size = int(1 * 2)
                            sample = int(sample[0])
                            f.write(data_size.to_bytes(4, byteorder='little', signed=False))
                            f.write(sample.to_bytes(2, byteorder='little', signed=True))
                        elif (sensor == 'pressure'):
                            # These are single axis sensors with int32 (4 byte) values
                            assert(len(sample) == 1)
                            data_size = int(1 * 4)
                            sample = int(sample[0])
                            f.write(data_size.to_bytes(4, byteorder='little', signed=True))
                            f.write(sample.to_bytes(4, byteorder='little', signed=True))

                        cur_ts += ts_step


# Create and write WAV file with parameters from metadata file
def write_SDS_AudioWAV(framerate, data, meta_data):
    raw_data = data["raw_data"]
    n_channels = len(meta_data)
    d_type = getDataType(meta_data[0]["type"])
    sample_width = calcsize(d_type)

    # Set audio parameters and write binary data to file
    wave_file.setnchannels(n_channels)
    wave_file.setsampwidth(sample_width)
    wave_file.setframerate(framerate)
    wave_file.writeframes(raw_data)
    wave_file.close()


def in_file(in_file):
    global in_extension

    # Check if all input files have a supported file extension
    in_extension.append(in_file.split('.')[-1])
    if in_extension[1:] != in_extension[:-1]:
        sys.exit(f"Input file extensions are not the same: {in_extension}")

    return in_file


def out_file(out_file):
    global out_extension

    # Check if all output files have a supported file extension
    out_extension.append(out_file.split('.')[-1])
    if out_extension[1:] != out_extension[:-1]:
        sys.exit(f"Output file extensions are not the same: {out_extension}")

    return out_file


# Main function
def main():
    global in_extension, out_extension

    in_extension  = []
    out_extension = []

    # Parse arguments
    formatter = lambda prog: argparse.HelpFormatter(prog, max_help_position=60)
    parser = argparse.ArgumentParser(description="Convert from or to SDS files using selected data conversion format",
                                     formatter_class=formatter)

    subparsers = parser.add_subparsers(dest="convert_format", help="Data conversion format", required=True)

    # Audio WAV
    parser_audio_wav = subparsers.add_parser("audio_wav", formatter_class=formatter)
    parser_audio_wav_required = parser_audio_wav.add_argument_group("required")
    parser_audio_wav_required.add_argument("-i", dest="in_file", metavar="<input_file>",
                                           help="Input file", nargs="+", type=in_file, required=True)
    parser_audio_wav_required.add_argument("-o", dest="out_file", metavar="<output_file>",
                                           help="Output file", type=out_file, required=True)
    parser_audio_wav_optional = parser_audio_wav.add_argument_group("optional")
    parser_audio_wav_optional.add_argument("-y", dest="yaml", metavar="<yaml_file>",
                                           help="YAML sensor description file", nargs="+", default=None)

    # Simple CSV
    parser_simple_csv = subparsers.add_parser("simple_csv", formatter_class=formatter)
    parser_simple_csv_required = parser_simple_csv.add_argument_group("required")
    parser_simple_csv_required.add_argument("-i", dest="in_file", metavar="<input_file>",
                                            help="Input file", nargs="+", type=in_file, required=True)
    parser_simple_csv_required.add_argument("-o", dest="out_file", metavar="<output_file>",
                                            help="Output file", type=out_file, required=True)
    parser_simple_csv_optional = parser_simple_csv.add_argument_group("optional")
    parser_simple_csv_optional.add_argument("-y", dest="yaml", metavar="<yaml_file>",
                                            help="YAML sensor description file", nargs="+", default=None)
    parser_simple_csv_optional.add_argument("--normalize", dest="normalize",
                                            help="Normalize timestamps so they start with 0", action="store_true")
    parser_simple_csv_optional.add_argument("--start-tick", dest="start_tick", metavar="<start-tick>",
                                            help="Exported data start tick (default: %(default)s)", type=float, default=None)
    parser_simple_csv_optional.add_argument("--stop-tick", dest="stop_tick", metavar="<stop-tick>",
                                            help="Exported data stop tick (default: %(default)s)", type=float, default=None)

    # Qeexo V2 CSV
    parser_qeexo_v2_csv = subparsers.add_parser("qeexo_v2_csv", formatter_class=formatter)
    parser_qeexo_v2_csv_required = parser_qeexo_v2_csv.add_argument_group("required")
    parser_qeexo_v2_csv_required.add_argument("-i", dest="in_file", metavar="<input_file>",
                                              help="Input file", nargs="+", type=in_file, required=True)
    parser_qeexo_v2_csv_required.add_argument("-o", dest="out_file", metavar="<output_file>",
                                              help="Output file", type=out_file, required=True)
    parser_qeexo_v2_csv_optional = parser_qeexo_v2_csv.add_argument_group("optional")
    parser_qeexo_v2_csv_optional.add_argument("-y", dest="yaml", metavar="<yaml_file>",
                                              help="YAML sensor description file", nargs="+", default=None)
    parser_qeexo_v2_csv_optional.add_argument("--normalize", dest="normalize",
                                              help="Normalize timestamps so they start with 0", action="store_true")
    parser_qeexo_v2_csv_optional.add_argument("--start-tick", dest="start_tick", metavar="<start-tick>",
                                              help="Exported data start tick (default: %(default)s)", type=float, default=None)
    parser_qeexo_v2_csv_optional.add_argument("--stop-tick", dest="stop_tick", metavar="<stop-tick>",
                                              help="Exported data stop tick (default: %(default)s)", type=float, default=None)
    parser_qeexo_v2_csv_optional.add_argument("--label", dest="label", metavar="'label'",
                                              help="Qeexo class label for sensor data (default: %(default)s)", default=None)
    parser_qeexo_v2_csv_optional.add_argument("--interval", dest="interval", metavar="<interval>",
                                              help="Qeexo timestamp interval in ms (default: %(default)s)", type=int, default=50)
    parser_qeexo_v2_csv_optional.add_argument("--sds_index", dest="sds_index", metavar="<sds_index>", 
                                              help="SDS file index to write (default: <sensor>.%(default)s.sds)", type=int, default=0)

    args = parser.parse_args()

    if not isinstance(args.in_file, list):
        args.in_file = [args.in_file]
    if not isinstance(args.out_file, list):
        args.out_file = [args.out_file]

    # Determine conversion direction
    if 'sds' in in_extension:
        sds_files   = args.in_file
        other_files = args.out_file
        if args.yaml is None:
            sys.exit("YAML file is missing from arguments. [-y <yaml_file>]")
        else:
            # Load data from YAML file
            sensor_name = []
            meta_data = {}
            sensor_frequency = {}
            for filename in args.yaml:
                try:
                    with open(filename, "r") as file:
                        yaml_data = yaml.load(file, Loader=yaml.FullLoader)["sds"]
                        sensor_name.append(yaml_data["name"])
                        sensor_frequency[sensor_name[-1]] = yaml_data["frequency"]
                        meta_data[sensor_name[-1]] = yaml_data["content"]
                except Exception as e:
                    sys.exit(f"Error loading YAML file: {e}")
    elif 'sds' in out_extension:
        other_files = args.in_file
        sds_files   = args.out_file
    else:
        sys.exit("SDS file is missing from arguments.")

    # Load data from SDS file
    if 'sds' in in_extension:
        Record = RecordManager()
        data = {}
        i = 0
        for filename in sds_files:
            try:
                with open(filename, "rb") as file:
                    data[sensor_name[i]] = Record.getData(file)
            except Exception as e:
                sys.exit(f"Error loading SDS file: {e}")
            i += 1

    # CSV
    if "csv" in args.convert_format:
        filename = other_files[0].split('.csv')[0]

        if 'sds' in in_extension:
            createCSV(filename)

            if args.convert_format == "qeexo_v2_csv":
                # Check if interval is zero
                if args.interval == 0:
                    sys.exit(f"Invalid interval option: {args.interval} ms")
                write_SDS_QeexoV2CSV(args, data, meta_data)
            elif args.convert_format == "simple_csv":
                # Only used for one sensor
                if (len(args.yaml) > 1) or (len(sds_files) > 1):
                    sys.exit("Simple CSV file format only supports 1 metadata and 1 SDS file")
                write_SDS_SimpleCSV(args, data[sensor_name[0]], meta_data[sensor_name[0]])
        else:
            readCSV(filename)

            if args.convert_format == "qeexo_v2_csv":
                write_QeexoV2CSV_SDS(args.sds_index)
            elif args.convert_format == "simple_csv":
                sys.exit('Simple CSV to SDS conversion is not supported.')

    # WAV
    elif "wav" in args.convert_format:
        if 'sds' in in_extension:
            filename = other_files.split('.wav')[0]
            createWAV(filename)

            if args.convert_format == "audio_wav":
                # Only used for one sensor
                if (len(args.yaml) > 1) or (len(sds_files) > 1):
                    sys.exit("Audio WAV file format only supports 1 metadata and 1 SDS file")
                write_SDS_AudioWAV(sensor_frequency[sensor_name[0]], data[sensor_name[0]], meta_data[sensor_name[0]])
        else:
            sys.exit('WAV to SDS conversion is not supported.')


if __name__ == "__main__":
    main()
