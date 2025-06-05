# Utilities

<!-- markdownlint-disable MD013 -->
<!-- markdownlint-disable MD036 -->
<!-- markdownlint-disable MD024 -->

The SDS-Framework includes the following utilities that are implemented in Python.

- [**SDSIO-Server:**](#sdsio-server) enables recording and playback of SDS data files via socket (TCP/IP), USB (Bulk transfer) or serial (UART) connection.
- [**SDS-View:**](#sds-view) graphical data viewer for SDS data files.
- [**SDS-Convert:**](#sds-convert) convert SDS data files into CSV, Qeexo V2 CSV, or WAV format.
- [**SDS-Check:**](#sds-check) check SDS data files for correctness and consistency.

## Requirements

- **Python 3.9** or later with packages:
    - ifaddr
    - matplotlib
    - numpy
    - pandas
    - pyyaml
    - pyserial
    - libusb1

## Setup

- Verify the installed Python version with:

```txt
python --version
```

- Option 1: Use a Python environment:

```txt
python -m venv sds                 // create environment with name sds
sds\Scripts\activate
pip install -r requirements.txt
```

- Option 2: Install the required Python packages with `pip`:

```txt
pip install ifaddr matplotlib numpy pandas pyyaml pyserial libusb1
```

## SDSIO-Server

The Python utility [**SDSIO-Server**](https://github.com/ARM-software/SDS-Framework/tree/main/utilities) enables recording and playback of SDS data files via socket (TCP/IP), USB (Bulk transfer), or serial (UART) connection.
It communicates with the target using these [SDSIO Client interfaces](https://github.com/ARM-software/SDS-Framework/tree/main/sds/source/sdsio/client):

- [serial/usart](https://github.com/ARM-software/SDS-Framework/tree/main/sds/source/sdsio/client/serial/usart) for serial communication via CMSIS-Driver USART.
- [socket](https://github.com/ARM-software/SDS-Framework/tree/main/sds/source/sdsio/client/socket) for TCP/IP communication via IoT Socket using MDK-Middleware, LwIP, or CMSIS-Driver WiFi.
- [usb/bulk](https://github.com/ARM-software/SDS-Framework/tree/main/sds/source/sdsio/client/usb) for communication via USB Bulk transfer using MDK-Middleware.

The SDS data stream is recorded to files with the following naming convention:

`<name>.<index>.sds`

- `<name>` is the name of the I/O stream specified with the function `sdsRecOpen` or `sdsPlayOpen` on the target.
- `<index>` is the zero-based index which is incremented for each subsequent recording.
- For more details see [Filenames section](theory.md#filenames)

The data content of the `<name>.<index>.sds` is described with metadata file `<name>.sds.yml` in [YAML format](https://github.com/ARM-software/SDS-Framework/tree/main/schema).

### Usage

- [Setup](#setup) the Python environment.
- Depending on the SDS interface used on the target use either [Serial Mode](#serial-mode), [Socket Mode](#socket-mode) or [USB Mode](#usb-mode) as described below.
- The SDSIO_Server terminates with `Ctrl+C`.

#### Serial Mode

```txt
usage: sdsio-server.py serial [-h] -p <Serial Port> [--baudrate <Baudrate>] [--parity <Parity>] [--stopbits <Stop bits>] [--connect-timeout <Timeout>] [--workdir <Work dir>]

optional arguments:
  -h, --help                   show this help message and exit

required:
  -p <Serial Port>             Serial port

optional:
  --baudrate <Baudrate>        Baudrate (default: 115200)
  --parity <Parity>            Parity: N=None, E=Even, O=Odd, M=Mark, S=Space (default: N)
  --stopbits <Stop bits>       Stop bits: 1, 1.5, 2 (default: 1)
  --connect-timeout <Timeout>  Serial port connection timeout in seconds (default: no timeout)
  --workdir <Work dir>         Directory for SDS files (default: current directory)
```

**Example:**

```bash
python sdsio-server.py serial -p COM0 --baudrate 115200 --workdir ./work_dir
```

#### Socket Mode

```txt
usage: sdsio-server.py socket [-h] [--ipaddr <IP> | --interface <Interface>] [--port <TCP Port>] [--workdir <Work dir>]

optional arguments:
  -h, --help               show this help message and exit

optional:
  --ipaddr <IP>            Server IP address (cannot be used with --interface)
  --interface <Interface>  Network interface (cannot be used with --ipaddr)
  --port <TCP Port>        TCP port (default: 5050)
  --workdir <Work dir>     Directory for SDS files (default: current directory)
```

!!! Notes
  - The `--ipaddr` and `--interface` options are mutually exclusive.
  - SDSIO Server only supports IPv4 addresses.

**Example:**

For Microsoft Windows (using default computer IP):
```bash
python sdsio-server.py socket --workdir ./work_dir
```

For Linux:
```bash
python sdsio-server.py socket --interface eth0 --workdir ./work_dir
```

#### USB Mode

```txt
usage: sdsio-server.py usb [-h] [--workdir <Work dir>] [--high-priority]

optional arguments:
  -h, --help            show this help message and exit

optional:
  --workdir <Work dir>  Directory for SDS files (default: current directory)
  --high-priority       Enable high-priority threading for USB server (default: off)
```

!!! Notes
  - For more reliable operation at higher data transfer rates, it is recommended to enable the `--high-priority` option. This increases the thread priority of the SDSIO-Server process.
  - When using `--high-priority`, elevated privileges are required depending on your operating system:
      - **Windows**: Run the Python script as an administrator.
      - **macOS/Linux**: Execute the script with `sudo` or ensure the user has sufficient permissions.

**Example:**

```bash
python sdsio-server.py usb --workdir ./work_dir
```

## SDS-View

The Python utility [**SDSIO-View**](https://github.com/ARM-software/SDS-Framework/tree/main/utilities) outputs a time-based plot of SDS data files (`<name>.<index>.sds`) based on the meta-data file (`<name>.sds.yml`).

The horizontal time scale is derived from the number of data points in a recording and frequency provided in the metadata description. All plots form a single recording will be displayed on the same figure (shared vertical scale).

If there are 3 values described in the metadata file, an optional 3D view may be displayed.  

### Limitations

- Data in recording must all be of the same type (float, uint32_t, uint16_t, ...)

### Usage

- [Setup](#setup) the Python environment.
- Invoke the tool as explained below.

```txt
usage: sds-view.py [-h] -y <yaml_file> -s <sds_file> [<sds_file> ...] [--3D]

View SDS data

options:
  -h, --help                      show this help message and exit

required:
  -y <yaml_file>                  YAML sensor description file
  -s <sds_file> [<sds_file> ...]  SDS data recording file

optional:
  --3D                            Plot 3D view in addition to normal 2D
```

**Example:**

```bash
python sds-view.py -y test/Gyroscope.sds.yml -s test/Gyroscope.0.sds
```

**Example display:**

![Example output](images/SDS-View.png)

## SDS-Convert

The Python utility [**SDSIO-Convert**](https://github.com/ARM-software/SDS-Framework/tree/main/utilities) converts SDS data files to selected format based on description in metadata (YAML) files.  

### Usage

- [Setup](#setup) the Python environment.
- Depending on the required format use the tool as shown below.

#### Audio WAV

The `audio_wav` mode converts raw microphone data from `.sds` files into a standard RIFF/WAV file using linear
PCM encoding. The conversion process involves appending a WAV header, generated from parameters specified in the
associated metadata `.yml` file, to the raw audio data extracted from the `.sds` stream. The metadata defines
essential audio parameters such as channel configuration (mono or stereo), sample rate (frame rate), and sample
width (bit depth).

```txt
usage: sds-convert.py audio_wav [-h] -i <input_file> [<input_file> ...] -o <output_file> [-y <yaml_file> [<yaml_file> ...]]

options:
  -h, --help                          show this help message and exit

required:
  -i <input_file> [<input_file> ...]  Input file
  -o <output_file>                    Output file

optional:
  -y <yaml_file> [<yaml_file> ...]    YAML sensor description file
```

!!! Note
    - The metadata and SDS data file pairs must be passed as arguments in the same order to decode the data correctly.
    - The tool expects the SDS stream to be strictly audio — no header markers or custom formatting.

**Example of metadata yml file for mono microphone:**

```yml
sds:
  name: Microphone
  description: Mono microphone with 16kHz sample rate
  frequency: 16000
  content:
  - value: Mono
    type: int16_t
```

**Example of metadata yml file for stereo microphone:**

```yml
sds:
  name: Microphone
  description: Stereo microphone with 16kHz sample rate
  frequency: 16000
  content:
  - value: Left channel
    type: int16_t
  - value: Right channel
    type: int16_t
```

**Example:**

```bash
python sds-convert.py audio_wav -i Microphone.0.sds -o microphone.wav -y Microphone.sds.yml
```

#### Simple CSV

The `simple_csv` mode converts sensor data from `.sds` files into a human-readable CSV format.
This mode is designed for exporting data from a single sensor. If the sensor has multiple
channels, each channel will appear as a separate column in the output CSV.

Timestamps are represented in floating-point format, in seconds. Using the `--normalize` flag causes
all timestamps in the input file to be offset so that the first timestamp is `0`.

Users may specify a time range selection of the input data to be processed using the following flags:

- `--start-timestamp <timestamp>`: Starting input data timestamp in floating-point format, in seconds.
- `--stop-timestamp <timestamp>`: Stopping input data timestamp in floating-point format, in seconds.

```txt
usage: sds-convert.py simple_csv [-h] -i <input_file> [<input_file> ...] -o <output_file> [-y <yaml_file> [<yaml_file> ...]] [--normalize] [--start-timestamp <timestamp>] [--stop-timestamp <timestamp>]

options:
  -h, --help                          show this help message and exit

required:
  -i <input_file> [<input_file> ...]  Input file
  -o <output_file>                    Output file

optional:
  -y <yaml_file> [<yaml_file> ...]    YAML sensor description file
  --normalize                         Normalize timestamps so they start with 0
  --start-timestamp <timestamp>       Starting input data timestamp, in seconds (default: None)
  --stop-timestamp <timestamp>        Stopping input data timestamp, in seconds (default: None)
```

!!! Note
    - The metadata and SDS data file pairs must be passed as arguments in the same order to decode the data correctly.
    - Current implementation assumes that the tick frequency is `1ms` and does not use the `tick-frequency` value from the metadata file.

**Example of metadata yml file for gyroscope:**

```yml
sds:
  name: Gyroscope
  description: Gyroscope with 1667Hz sample rate
  frequency: 1667
  content:
  - value: x
    type: int16_t
    scale: 0.07
    unit: dps
  - value: y
    type: int16_t
    scale: 0.07
    unit: dps
  - value: z
    type: int16_t
    scale: 0.07
    unit: dps
```

**Example:**

```bash
python sds-convert.py simple_csv  -i Gyroscope.0.sds -o gyroscope_simple.csv -y Gyroscope.sds.yml --normalize --start-tick 0.2 --stop-tick 0.3
```

#### Qeexo V2 CSV

Timestamps are represented in integer format, in milliseconds. Using the `--normalize` flag causes
all timestamps in the input file to be offset so that the first timestamp is `0`.

Users may specify a time range selection of the input data to be processed using the following flags:

- `--start-timestamp <timestamp>`: Starting input data timestamp in integer format, in milliseconds.
- `--stop-timestamp <timestamp>`: Stopping input data timestamp in integer format, in milliseconds.

By default, the output file will have raw timestamps in integer format, in milliseconds.
The default output timestamp interval is set to `50 ms`.
To override this setting use the `--interval <ms>` flag, where `<ms>` is the desired interval in milliseconds.

An optional label can be added to the output by providing a string argument to the `--label <text>` flag.
This `<text>` will populate the label column in the output file.

Link to [Qeexo V2 CSV format specification](https://docs.qeexo.com/guides/userguides/data-management#2-1-Data-format-specification).

```txt
usage: sds-convert.py qeexo_v2_csv [-h] -i <input_file> [<input_file> ...] -o <output_file> [-y <yaml_file> [<yaml_file> ...]] [--normalize] [--start-timestamp <timestamp>] [--stop-timestamp <timestamp>] [--label 'label'] [--interval <interval>] [--sds_index <sds_index>]

options:
  -h, --help                          show this help message and exit

required:
  -i <input_file> [<input_file> ...]  Input file
  -o <output_file>                    Output file

optional:
  -y <yaml_file> [<yaml_file> ...]    YAML sensor description file
  --normalize                         Normalize timestamps so they start with 0
  --start-timestamp <timestamp>       Starting input data timestamp, in ms (default: None)
  --stop-timestamp <timestamp>        Stopping input data timestamp, in ms (default: None)
  --label 'label'                     Qeexo class label for sensor data (default: None)
  --interval <interval>               Qeexo timestamp interval, in ms (default: 50)
  --sds_index <sds_index>             SDS file index to write (default: <sensor>.0.sds)
```

!!! Note
    - The metadata and SDS data file pairs must be passed as arguments in the same order to decode the data correctly.
    - Current implementation assumes that the tick frequency is `1ms` and does not use the `tick-frequency` value from the metadata file.

**Example of metadata yml file for accelerometer:**

```yml
sds:
  name: Accelerometer
  description: Accelerometer with 1667Hz sample rate
  frequency: 1667
  content:
  - value: x
    type: int16_t
    scale: 0.000061
    unit: G
  - value: y
    type: int16_t
    scale: 0.000061
    unit: G
  - value: z
    type: int16_t
    scale: 0.000061
    unit: G
```

**Examples:**

Convert **SDS data** files to **Qeexo V2 CSV** files:

```bash
python sds-convert.py qeexo_v2_csv -i Gyroscope.0.sds Accelerometer.0.sds -o sensor_fusion.csv -y Gyroscope.sds.yaml Accelerometer.sds.yaml --normalize --start-tick 200 --stop-tick 300
```

Convert **Qeexo V2 CSV** files to **SDS data** files:

```bash
python sds-convert qeexo_v2_csv -i accelerometer_data.csv -o accelerometer.sds
```

## SDS-Check

The Python utility [**SDS-Check**](https://github.com/ARM-software/SDS-Framework/tree/main/utilities) checks SDS data files for correctness and consistency.  

The following checks are performed:

- [Size consistency check](#size-consistency-check): data size of all records should match the size of the SDS file.
- [Timestamp consistency check](#timestamp-consistency-check): verify that timestamps of the records are in ascending order.
- [Jitter check](#jitter-check): print the record with the largest deviation from the average timestamp interval.
- [Delta time check](#delta-time-check): find the record with the largest timestamps difference from the following record.
- [Duplicate timestamp check](#duplicate-timestamp-check): find records that have identical timestamps.

### Usage

- [Setup](#setup) the Python environment.
- Invoke the tool as explained below.

```txt
usage: sds-check.py [-h] -s <sds_file>

SDS data validation

options:
  -h, --help     show this help message and exit

required:
  -s <sds_file>  SDS data recording file
```

**Example:**

```txt
python sds-check.py -s Accelerometer.0.sds
File     : Accelerometer.0.sds
DataSize : 156.020 bytes
Records  : 289
BlockSize: 532 bytes
Largest  : 552 bytes
Smallest : 462 bytes
Interval : 50 ms
DataRate : 10.640 byte/s
Jitter   : 0 ms
Validation passed
```

!!! Note
    The time values assume an tick rate of 1000 Hz.

### Summary Report

After processing the SDS data file, the SDS-Check utility prints a summary report with statistics:

- **DataSize**:  total size of the data in bytes
- **Records**:   total number of records
- **BlockSize**: average block size of a data record
- **Largest**:   largest block size, if different from the average block size (optional)
- **Smallest**:  smallest block size, if different from the average block size (optional)
- **Interval**:  time interval of the recording in milliseconds
- **DataRate**:  recorded data rate in bytes per second
- **Jitter**:    deviation from the expected timestamps
- **DeltaTime**: largest difference of the neighboring timestamps, if deviating from the recording interval (optional)
- **DupStamps**: number of duplicated timestamps, if found (optional)

### Size consistency check

This check processes the SDS data records and calculates the total size of the SDS data.
It is the sum of all data records (header + data). This data size should match the size of
the SDS file.

If the sizes do not match this error is printed:

```txt
Error: File size mismatch. Expected 360 bytes, but file contains 363 bytes.
```

### Timestamp consistency check

This check processes the SDS records and ensures that the timestamps recorded in the records
are arranged in ascending order. If the utility detects that the timestamp of the subsequent
data record is lower than the current one, this error is printed:

```txt
Error: Timestamp not in ascending order in record 23.
```

### Jitter check

This check processes the SDS data records and searches for a maximum deviation of the recorded
timestamps from the expected ones. If the deviation is found, the record number is saved. The
maximum deviation is evaluated as **jitter** and printed out in the summary report.

```txt
File     : Gyroscope.0.sds
DataSize : 153.334 bytes
Records  : 284
BlockSize: 532 bytes
Largest  : 606 bytes
Smallest : 444 bytes
Interval : 50 ms
DataRate : 10.640 byte/s
Jitter   : 0 ms
Validation passed
```

### Delta time check

This check processes the SDS records and tries to find the largest difference in timestamps
between two neighboring records, called **DeltaTime**.

For normally recorded files, the delta time and the recording interval are identical, so no information
about the delta time status is printed. If the delta time and the recording interval are not identical,
i.e. a difference is detected, the record number is also saved and the **DeltaTime** is printed in
the summary report.

```txt
File     : Temperature.0.sds
DataSize : 360 bytes
Records  : 30
BlockSize: 4 bytes
Interval : 1.024 ms
DataRate : 4 byte/s
Jitter   : 59 ms, record 19
DeltaTime: 1.050 ms, record 2
Validation passed
```

This is not an error, but a report of an anomaly. If the time delta is significantly longer than the
sampling interval —e.g., several times longer— it may indicate that one or more data records are missing
from the recorded file.

### Duplicate timestamp check

This check processes the SDS records in search of duplicated timestamps.
This means that the same timestamp is used in several consecutive data records.

This may indicate that the recording loop in an embedded application is not set up correctly. It is also
possible that duplicate timestamps are caused by unexpected thread delays in the embedded application.

Duplicate timestamps are unusual in typical recording files. If multiple timestamps with the same value
are found in the SDS file, **DupStamps** will be added in the summary report.

```txt
File     : DataInput.0.sds
DataSize : 17.509.440 bytes
Records  : 47.580
BlockSize: 360 bytes
Interval : 1 ms
DataRate : 360.000 byte/s
Jitter   : 4 ms, record 4
DeltaTime: 5 ms, record 5
DupStamps: 4, record 1
Validation passed
```

This is not an error, but a report of an anomaly. The report contains the number of records with
the same timestamp and the position in the SDS file where the anomaly was detected (record number).

!!! Note 
    Only the first occurrence of a duplicate timestamp is reported.
