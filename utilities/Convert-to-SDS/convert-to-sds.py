# Copyright (c) 2023 Qeexo Company. All rights reserved.
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

# Python Convert from CSV to SDS
import argparse
import json

import pandas as pd

# Potential AutoML CSV V2 columns that are not sensors, and so should be ignored.
EXCLUDE_QX_CSVV2_COLS = ('timestamp', 'index', 'event', 'label', 'data_type', 'seconds', 'recording_id', 'event_id')

def main():
    ''' Main entry point. Parse the arguments and call the appropriate functions.'''
    formatter = lambda prog: argparse.HelpFormatter(prog,max_help_position=60)
    parser = argparse.ArgumentParser(description="Convert CSV Sensor Data to SDS",
                                     formatter_class=formatter)
    required = parser.add_argument_group("required")
    required.add_argument("-i", dest="input", metavar="<input_csv>",
                          help="Input Sensor CSV file.", 
                          type=argparse.FileType('r'), required=True)
    required.add_argument("-f", dest="format", choices=['qeexo_v2_csv'],
                          help="Input CSV Data Format", required=True)
    optional = parser.add_argument_group("optional")
    optional.add_argument("--sds_id", dest="sds_id", metavar="<sds_file_id>",
                          type=int, default=0, help="Id number for SDS files to write."+
                          "SDS files will be written to filenames: <sensor>.<sds_id>.sds")

    args = parser.parse_args()

    data = pd.read_csv(args.input)
    sensors = data.columns.tolist()
    sensors = set(sensors) - set(EXCLUDE_QX_CSVV2_COLS)
    for sensor in sensors:
        if "qeexo_v2_csv" in args.format:
            write_qxv2csv_sds_file(data, sensor, args.sds_id)

def write_qxv2csv_sds_file(data, sensor, recording_id):
    '''Main function to write the SDS file from a column of sensor data in Qeexo CSV V2 format.
    Write the data from data[sensor] column to file: <sensor>.<recording_id>.sds'''
    filename = f'{sensor}.{recording_id}.sds'
    fout = open(filename, 'wb')
    for index, row in data.iterrows():
        start_ts = int(row['timestamp'])
        # Sensor data is a string which is a JSON array containing all of the samples at this 50ms 
        # timestamp block
        allsamples = row[sensor]
        data = json.loads(allsamples)
        if len(data)>0:
            # We have data to write out.
            ts_step = 50/float(len(data))
            cur_ts = float(start_ts)
            for sample in data:
                write_qxv2csv_sds_record(fout, sensor, round(cur_ts), sample)
                cur_ts += ts_step
    fout.close()


def write_qxv2csv_sds_record(fout, sensor, ts, sample):
    '''Write the binary record for this single sensor sample to SDS format as specified in SDS-Framework/schema
    The binary schema is:
    - timestamp: record timestamp in tick-frequency (32-bit unsigned integer, little endian)
    - data size: number of data bytes in the record (32-bit unsigned integer, little endian)
    - binary data: SDS stream (little endian, no padding) as described with the *.sds.yml file.
    You can find the sensor types and data types per sensor for Qeexo V2 CSV here: 
    https://docs.qeexo.com/guides/userguides/data-management#mainDataManagement-2-1-Data-format-specification2-1Dataformatspecification
    '''
    ts = int(ts)
    fout.write(ts.to_bytes(4, byteorder='little', signed=False))
    if sensor=='accel' or sensor=='gyro' or sensor=='magno':
        # These are each 3-axes sensors (x,y,z) with int16 (2 byte) values
        assert(len(sample)==3)
        data_size = int(3 * 2)
        fout.write(data_size.to_bytes(4, byteorder='little', signed=False))
        sample_x = int(sample[0])
        sample_y = int(sample[1])
        sample_z = int(sample[2])
        fout.write(sample_x.to_bytes(2, byteorder='little', signed=True))
        fout.write(sample_y.to_bytes(2, byteorder='little', signed=True))
        fout.write(sample_z.to_bytes(2, byteorder='little', signed=True))
    elif sensor=='temperature' or sensor=='humidity' or sensor=='microphone':
        # These are single axes sensors with int16 (2 byte) values
        assert(len(sample)==1)
        data_size = int(1 * 2)
        fout.write(data_size.to_bytes(4, byteorder='little', signed=False))
        sample = int(sample[0])
        fout.write(sample.to_bytes(2, byteorder='little', signed=True))
    elif sensor=='pressure':
        # These are single axis sensors with int32 (4 byte) values
        assert(len(sample)==1)
        data_size = int(1 * 4)
        fout.write(data_size.to_bytes(4, byteorder='little', signed=True))
        sample = int(sample[0])
        fout.write(sample.to_bytes(4, byteorder='little', signed=True))


if __name__ == '__main__':
    main()
