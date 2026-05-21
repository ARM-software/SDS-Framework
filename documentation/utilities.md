# Utilities

<!-- markdownlint-disable MD013 -->
<!-- markdownlint-disable MD036 -->
<!-- markdownlint-disable MD024 -->

The SDS-Framework pack includes in the folder `/utilities` several utilities that are implemented in Python.
Install **Python** and packages listed in file `/utilities/requirements.txt` to run these utilities:

- [SDSIO Control File: `*.sdsio.yml`](#sdsio-control-file-sdsioyml) options for SDSIO-Server and for the AVH FVP VSI3 simulation interface (`sdsio.yml`).
- [**SDSIO-Server:**](#sdsio-server) enables recording and playback of SDS data files via USB, socket (TCP/IP),
  Segger RTT or serial (UART) connection.
- [**SDS-View:**](#sds-view) graphical data viewer for SDS data files.
- [**SDS-Convert:**](#sds-convert) convert SDS data files into CSV, Qeexo V2 CSV, or WAV format.
- [**SDS-Check:**](#sds-check) check SDS data files for correctness and consistency.

## Setup

!!! Note
    - The example below uses version 3.0.0 of the SDS pack. Adapt the version to match the installed SDS pack on your computer.

Perform the following steps to setup the Python environment for using the SDS utilities.

- [Install Python](https://www.python.org/downloads/) or verify the version with:

```bash
>python --version
```

- Navigate in the folder `SDS/<version>/utilities` and install the required Python packages with `pip`:

```bash
>cd %CMSIS_PACK_ROOT%/ARM/SDS/3.0.0/utilities
>pip install -r requirements.txt
```

- Add to the system **Path** environment variable the path to the `%CMSIS_PACK_ROOT%/ARM/SDS/3.0.0/utilities` folder.

!!! Note
    - `%CMSIS_PACK_ROOT%` is just a placeholder for the Pack location on your local PC. The **Path** variable must be extended by the absolute path to the `utilities` folder.

### Windows

- On Windows, ensure that the  environment variable **PATHEXT** contains the extension `.PY`.

!!! Tip
    - When the **Path** environment variable is configured, you may simply start the utilities by using its name. For example entering `>sdsio-server` starts the utility.

## SDSIO Control File: `*.sdsio.yml`

The SDSIO data file I/O can be configured using a YAML control file with the root node `sdsio:`.

- For [SDSIO-Server](#sdsio-server), the control file is specified with `--control <*.sdsio.yml>`.
- For AVH FVP simulation (VSI3), the simulator reads `sdsio.yml` (or `*.sdsio.yml`) from the simulator working directory. See [SDSIO Interface](sdsio.md#layer-sdsio_fvp).

### `sdsio:`

`sdsio:`                                                    |              | Content
:-----------------------------------------------------------|:-------------|:------------------------------------
&nbsp;&nbsp;&nbsp; [`interface:`](#interface)               |   Optional   | SDSIO-Server only: specifies the interface used to connect to the target firmware (default: `usb`).
&nbsp;&nbsp;&nbsp; `workdir:`                               |   Optional   | Directory that stores `*.sds` files (default: current working directory). In AVH FVP simulation, relative paths are relative to the simulator working directory.
&nbsp;&nbsp;&nbsp; `metadir:`                               |   Optional   | Directory for metadata files (default: `workdir`). This key is currently not used by SDSIO-Server or the VSI3 simulation interface.
&nbsp;&nbsp;&nbsp; [`play:`](#play)                         |   Optional   | Playback step list that defines how `*.sds` files are played back (used in playback mode).
&nbsp;&nbsp;&nbsp; [`flag-info:`](#flag-info)               |   Optional   | Human readable labels for user flags. This key is currently not used by SDSIO-Server or the VSI3 simulation interface.

### `interface:`

!!! Note
    - The `interface:` settings are used by SDSIO-Server. The AVH FVP VSI3 simulation interface ignores `interface:`.

`interface:`                                                |              | Content
:-----------------------------------------------------------|:-------------|:------------------------------------
&nbsp;&nbsp;&nbsp; [`usb:`](#usb)                           |   Optional   | Configure USB bulk interface.
&nbsp;&nbsp;&nbsp; [`serial:`](#serial)                     |   Optional   | Configure serial (UART) interface.
&nbsp;&nbsp;&nbsp; [`socket:`](#socket)                     |   Optional   | Configure TCP socket interface.

#### `usb:`

`usb:`                                                      |              | Content
:-----------------------------------------------------------|:-------------|:------------------------------------
&nbsp;&nbsp;&nbsp; `high_priority:`                         |   Optional   | SDSIO-Server only: increase process priority (default: `false`).

#### `serial:`

`serial:`                                                   |              | Content
:-----------------------------------------------------------|:-------------|:------------------------------------
&nbsp;&nbsp;&nbsp; `port:`                                  | **Required** | Port name (examples: `COM3`, `ttyS0`, `ttyUSB1`, ...).
&nbsp;&nbsp;&nbsp; `baudrate:`                              |   Optional   | Baudrate (default: `115200`).
&nbsp;&nbsp;&nbsp; `parity:`                                |   Optional   | Parity bit: `none`, `even`, `odd`, `mark`, `space` (default: `none`).
&nbsp;&nbsp;&nbsp; `stopbits:`                              |   Optional   | Stop bits: `1`, `1.5`, `2` (default: `1`).

#### `socket:`

`socket:`                                                   |              | Content
:-----------------------------------------------------------|:-------------|:------------------------------------
&nbsp;&nbsp;&nbsp; `ipaddr:`                                |   Optional   | IPv4 address to bind to (example: `192.168.0.100`); cannot be used with `netif`.
&nbsp;&nbsp;&nbsp; `netif:`                                 |   Optional   | Network interface name (example: `eth0`); cannot be used with `ipaddr`.
&nbsp;&nbsp;&nbsp; `port:`                                  |   Optional   | TCP port number (default: `5050`).

!!! Note
    - The `ipaddr:` and `netif:` options are mutually exclusive.

### `play:`

The `play:` node specifies one or more playback steps.
Each playback step defines the list of labels to play back for each opened SDS stream.

For one playback step, all files of the `labels:` list are concatenated and appear as one data stream for the firmware.
A pause (where the data stream needs to be closed and opened again by the firmware) is created with another `- step:` section.

`play:`                                                     |              | Content
:-----------------------------------------------------------|:-------------|:------------------------------------
`- step:`                                                   |   Optional   | Descriptive text for the playback step.
&nbsp;&nbsp;&nbsp; `recdir:`                                |   Optional   | Recording directory for generated `*.p.sds` files during playback; relative paths are relative to `workdir:`.
&nbsp;&nbsp;&nbsp; `setflags:`                              |   Optional   | Set user flags (bitmask); example: `0x01`.
&nbsp;&nbsp;&nbsp; `clearflags:`                            |   Optional   | Clear user flags (bitmask); example: `0x01`.
&nbsp;&nbsp;&nbsp; `labels:`                                |   Optional   | List of label names that define the playback sequence.

!!! Note
    - The option name is `clearflags:` (older proposals used `clrflags:`).

### `flag-info:`

The `flag-info:` node provides human readable labels for the user flags 0..7.
This information is currently not used by SDSIO-Server; it is intended for UI tooling.

`flag-info:`                                                |              | Content
:-----------------------------------------------------------|:-------------|:------------------------------------
`- <flag-bit>:`                                             | **Required** | Maps the user flag bit position `0..7` to a human readable label.

**Example:**

```yml
sdsio:
  flag-info:
    - 0: Skip Algorithm A
    - 1: Test error occurred
    - 7: Reserved
```

### Example

```yml
sdsio:
  interface:
    socket:
      port: 5050

  workdir: ./SDS Recordings
  metadir: ./SDS Recordings

  play:
    - step: "Step 1"
      labels: [ case1, case2, case3 ]

    - step: "Step 2"
      recdir: test1
      setflags: 0x01
      clearflags: 0x02
      labels:
        - case1
        - case4
        - case3
```

## SDSIO-Server

The Python utility [**SDSIO-Server**](https://github.com/ARM-software/SDS-Framework/tree/main/utilities) enables recording and playback of SDS data files via socket (TCP/IP), USB (Bulk transfer), or serial (UART) connection.
It communicates with the target using these [SDSIO Client interfaces](https://github.com/ARM-software/SDS-Framework/tree/main/sds/sdsio/client):

- [serial/usart](https://github.com/ARM-software/SDS-Framework/tree/main/sds/sdsio/client/sdsio_client_serial.c) for serial communication via CMSIS-Driver USART.
- [socket](https://github.com/ARM-software/SDS-Framework/tree/main/sds/sdsio/client/sdsio_client_socket.c) for TCP/IP communication via IoT Socket using MDK-Middleware, LwIP, or CMSIS-Driver WiFi.
- [usb/bulk](https://github.com/ARM-software/SDS-Framework/tree/main/sds/sdsio/client/sdsio_client_usb_mdk.c) for communication via USB Bulk transfer using MDK-Middleware.

The SDS data stream is recorded to and played back from `*.sds` files with the following naming convention:

`<name>.<label>[.p].sds`  (where `[.p]` is optional and present only for recorded files in playback mode)

For more details see [Filenames section](theory.md#filenames)

The data content of the `<name>.<label>[.p].sds` is described with metadata file `<name>.sds.yml` in [YAML format](https://github.com/ARM-software/SDS-Framework/tree/main/schema).

### Usage

- [Setup](#setup) the Python environment.
- Configure via a YAML control file (`-c *.sdsio.yml`) or specify the interface directly on the command line.
- Terminate the server with `Ctrl+C` or by pressing `X`.

```txt
usage: sdsio-server.py [-h] [-V] [{socket | serial | usb} [if-opts]] [-c <*.sdsio.yml>] [general-opts]

SDSIO-Server: record and playback SDS data stream files over USB, socket, or serial interface.
Configure via *.sdsio.yml file or specify the interface parameters directly on the command line.

options:
  --help, -h                   Show this help message
  --version, -V                Show program's version number

interface (optional, default: usb; overrides interface in *.sdsio.yml):
  {socket | serial | usb}
    socket                     Run TCP socket server
    serial                     Run serial server
    usb                        Run USB bulk server

configuration:
  --control, -c <*.sdsio.yml>  Configure interface, SDS file directories, and playback steps

general-opts:
  --playback, -p               Start SDSIO-Server in playback mode (used in CI tests)
  --workdir <path>             Directory for SDS files (overrides *.sdsio.yml; default: current directory)
  --mon-port, -m <port>        Monitor control interface port
  --log, -l <file>             Redirect console output to a log file (for CI use)
  --verbose, -v                Enable debug messages
  --high-priority              Increase process priority for USB server (requires elevated privileges)
```

**Keyboard input (while running):**

| Key | Action
|-----|------------------
| R/r | Start recording
| P/p | Start playback
| S/s | Stop  recording/playback
| A-H | Set user flags 0-7
| a-h | Clear user flags 0-7
| X/x | Terminate server

#### Serial Mode

```txt
usage: sdsio-server.py serial [-h] --port <Serial Port> [--baudrate <Baudrate>] [--parity <Parity>] [--stopbits <Stop bits>] [--connect-timeout <Timeout>] [general-opts]

options:
  -h, --help                   show this help message and exit

if-opts (required):
  --port <Serial Port>         Serial port (required)

if-opts (optional):
  --baudrate <Baudrate>        Baudrate (default: 115200)
  --parity <Parity>            Parity: none, even, odd, mark, space (default: none)
  --stopbits <Stop bits>       Stop bits: 1, 1.5, 2 (default: 1)
  --connect-timeout <Timeout>  Serial port connection timeout in seconds (default: no timeout)
```

**Example:**

```bash
python sdsio-server.py serial --port COM0 --baudrate 115200 --workdir ./work_dir
```

#### Socket Mode

```txt
usage: sdsio-server.py socket [-h] [--ipaddr <IP> | --netif <Interface>] [--port <TCP Port>] [general-opts]

options:
  -h, --help             show this help message and exit

if-opts (optional):
  --ipaddr <IP>          Server IP address (example: 192.168.0.100), cannot be used with 'netif'
  --netif <Interface>    Network interface (example: eth0), cannot be used with 'ipaddr'
  --port <TCP Port>      TCP port number (default: 5050)
```

!!! Note
    - The `--ipaddr` and `--netif` options are mutually exclusive.
    - SDSIO-Server only supports IPv4 addresses.


!!! Note
    - The target device and the Host computer must be connected to the same network. With a standard network installation, the DHCP server assigns IP addresses automatically.
    - On **Windows**, a firewall may restrict socket connections. To allow the SDSIO-Server through the Windows Defender Firewall:
        - Open **Windows Security - Firewall & network protection - Allow an app through firewall**.
        - Click **Change settings**, then allow your Python runtime (`python.exe`) on the network profile you use (usually Private).
        - On a managed corporate PC, Group Policy or endpoint security may still block connections. Your IT administrator may need to whitelist the IP address and port.

**Example:**

For Microsoft Windows (using default computer IP):

```bash
python sdsio-server.py socket --workdir ./work_dir
```

For Linux:

```bash
python sdsio-server.py socket --netif eth0 --workdir ./work_dir
```

#### USB Mode

```txt
usage: sdsio-server.py usb [-h] [general-opts]

options:
  -h, --help             show this help message and exit
```

!!! Note
    - For more reliable operation at higher data transfer rates, it is recommended to enable the `--high-priority` general option. This increases the thread priority of the SDSIO-Server process.
    - When using `--high-priority`, elevated privileges are required depending on your operating system:
        - **Windows**: Run the Python script as an administrator.
        - **macOS/Linux**: Execute the script with `sudo` or ensure the user has sufficient permissions.

!!! Note
    - On **macOS**, the **libusb** system library is required. If not already installed, you can install it using with [Homebrew](https://brew.sh/):

    `>brew install libusb`

!!! Note
    - On **Linux**, access to USB devices from user space requires a udev rule by default.
      Create a udev rule file (e.g. `/etc/udev/rules.d/99-sdsio.rules`) with the
      vendor and product ID of the SDSIO Client device:

      `SUBSYSTEM=="usb", ATTRS{idVendor}=="XXXX", ATTRS{idProduct}=="XXXX", MODE="0666"`

      Then reload rules with `sudo udevadm control --reload && sudo udevadm trigger`.
      Use `dmesg` before and after plugging in the device to find the vendor/product IDs.

**Example:**

```bash
python sdsio-server.py usb --workdir ./work_dir
```

#### Examples

```bash
# Recommended: all config in YAML
python sdsio-server.py -c sdsio.yml

# Playback mode
python sdsio-server.py -c sdsio.yml --playback

# With VS Code SDS extension monitor
python sdsio-server.py -c sdsio.yml --mon-port 6060

# USB server with explicit work directory
python sdsio-server.py usb --workdir ./data

# TCP socket server
python sdsio-server.py socket --port 5050

# Serial server
python sdsio-server.py serial --port COM3 --baudrate 115200
```

## SDS-View

The Python utility [**SDS-View**](https://github.com/ARM-software/SDS-Framework/tree/main/utilities) generates a time-based plot
from data recorded in SDS files (`<name>.<label>.sds`) using the metadata provided in `<name>.sds.yml`.

The horizontal time scale is derived from the number of data points in a recording and frequency provided in the metadata description.
All plots form a single recording will be displayed on the same figure (shared vertical scale).

If there are 3 values described in the metadata file, an optional 3D view may be displayed.

### Limitations

- Data in recording must all be of the same type (float, uint32_t, uint16_t, ...)

### Usage

- [Setup](#setup) the Python environment.
- Invoke the tool as explained below.

```txt
usage: sds-view.py [-h]
                   -i <sds_file> [<sds_file> ...]
                   -y <yaml_file>
                   [--3D]

View SDS data

options:
  -h, --help                      Show this help message and exit

required:
  -i <sds_file> [<sds_file> ...]  SDS files
  -y <yaml_file>                  YAML metadata file

optional:
  --3D                            Plot 3D view in addition to normal 2D
```

**Example:**

```bash
python sds-view.py -i test/Gyroscope.0.sds -y test/Gyroscope.sds.yml
```

**Example display:**

![Example output](images/SDS-View.png)

## SDS-Convert

The Python utility [**SDS-Convert**](https://github.com/ARM-software/SDS-Framework/tree/main/utilities) converts SDS files into the
selected output format based on the descriptions provided in the metadata (YAML) files.

### Usage

- [Setup](#setup) the Python environment.
- Depending on the required format use the tool as shown below.

#### Audio WAV

The `audio_wav` mode converts SDS file (`.sds`) containing raw microphone data into a standard RIFF/WAV file (`.wav`) using linear
PCM encoding. The conversion process involves appending a WAV header, generated from parameters specified in the
associated YAML metadata file (`.yml`), to the raw audio data extracted from the SDS file. The metadata defines
essential audio parameters such as channel configuration (mono or stereo), sample rate (frame rate), and sample
width (bit depth).

```txt
usage: sds-convert.py audio_wav [-h]
                                -i <input_file>
                                -o <output_file>
                                -y <yaml_file>

Convert SDS file to WAV audio files

options:
  -h, --help        show this help message and exit

required:
  -i <input_file>   Input file
  -o <output_file>  Output file
  -y <yaml_file>    YAML metadata file
```

!!! Note
    - The tool expects the SDS file to contain strictly audio data - no header markers or custom formatting.

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

The `simple_csv` mode converts SDS file (`.sds`) containing sensor data into a human-readable CSV file (`.csv`).

Timeslots are represented in floating-point format, in seconds. Using the `--normalize` parameter causes
all timeslots in the input file to be offset so that the first timeslot starts at `0`.

Users may specify a time range selection of the input data to be processed using the following parameters:

- `--start-timeslot <timeslot>`: Starting input data timeslot in floating-point format, in seconds.
- `--stop-timeslot <timeslot>` : Stopping input data timeslot in floating-point format, in seconds.

```txt
usage: sds-convert.py simple_csv [-h]
                                 -i <input_file>
                                 -o <output_file>
                                 -y <yaml_file>
                                 [--normalize]
                                 [--start-timeslot <timeslot>]
                                 [--stop-timeslot <timeslot>]

Convert SDS file to CSV file with timeslots and data columns

options:
  -h, --help                     show this help message and exit

required:
  -i <input_file>                Input file
  -o <output_file>               Output file
  -y <yaml_file>                 YAML metadata file

optional:
  --normalize                    Normalize timeslots so they start with 0
  --start-timeslot <timeslot>    Starting input data timeslot, in seconds (default: None)
  --stop-timeslot <timeslot>     Stopping input data timeslot, in seconds (default: None)
```

!!! Note
    - Current implementation assumes that the tick frequency is `1000 Hz` and does not use the `tick-frequency` value from the metadata file.

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
python sds-convert.py simple_csv -i Gyroscope.0.sds -o Gyroscope.csv -y Gyroscope.sds.yml --normalize --start-timeslot 0.2 --stop-timeslot 0.3
```

#### Qeexo V2 CSV

The `qeexo_v2_csv` mode converts between SDS files (`.sds`) containing sensor data and Qeexo V2 CSV files (`.csv`).

Link to [Qeexo V2 CSV format specification](https://docs.qeexo.com/guides/userguides/data-management#2-1-Data-format-specification).

Timeslots are represented in integer format, in milliseconds. Using the `--normalize` parameter causes
all timeslots in the input file to be offset so that the first timeslot starts at `0`.

Users may specify a time range selection of the input data to be processed using the following parameters:

- `--start-timeslot <timeslot>`: Starting input data timeslot in integer format, in milliseconds.
- `--stop-timeslot <timeslot>` : Stopping input data timeslot in integer format, in milliseconds.

By default, the output file will have raw timeslots in integer format, in milliseconds.
The default output timeslot interval is set to `50 ms`.
To override this setting use the `--interval <ms>` parameter, where `<ms>` is the desired interval in milliseconds.

An optional label can be added to the output by providing a string argument to the `--label <text>` parameter.
This `<text>` will populate the label column in the output file.

```txt
usage: sds-convert.py qeexo_v2_csv [-h]
                                   -i <input_file> [<input_file> ...]
                                   -o <output_file>
                                   [-y <yaml_file> [<yaml_file> ...]]
                                   [--normalize]
                                   [--start-timeslot <timeslot>]
                                   [--stop-timeslot <timeslot>]
                                   [--label 'label']
                                   [--interval <interval>]
                                   [--sds_index <sds_index>]

Convert between SDS and Qeexo AutoML V2 CSV files (supports multiple sensors)

options:
  -h, --help                          show this help message and exit

required:
  -i <input_file> [<input_file> ...]  Input files
  -o <output_file>                    Output file

optional:
  -y <yaml_file> [<yaml_file> ...]    YAML metadata files
  --normalize                         Normalize timeslots so they start with 0
  --start-timeslot <timeslot>         Starting input data timeslot, in ms (default: None)
  --stop-timeslot <timeslot>          Stopping input data timeslot, in ms (default: None)
  --label 'label'                     Qeexo class label for sensor data (default: None)
  --interval <interval>               Qeexo timeslot interval, in ms (default: 50)
  --sds_index <sds_index>             SDS file index to write (default: <sensor>.0.sds)
```

!!! Note
    - The SDS and metadata file pairs must be passed as arguments in the same order to decode the data correctly.
    - Current implementation assumes that the tick frequency is `1000 Hz` and does not use the `tick-frequency` value from the metadata file.

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

Convert **SDS** files to **Qeexo V2 CSV** files:

```bash
python sds-convert.py qeexo_v2_csv -i Gyroscope.0.sds Accelerometer.0.sds -o SensorFusion.csv -y Gyroscope.sds.yaml Accelerometer.sds.yaml --normalize --start-timeslot 200 --stop-timeslot 300
```

Convert **Qeexo V2 CSV** files to **SDS** files:

```bash
python sds-convert.py qeexo_v2_csv -i Accelerometer.csv -o Accelerometer.sds
```

#### Video

The `video` mode converts SDS file (`.sds`) containing video frames into a standard MP4 (H.264) file (`.mp4`).
Video frame format shall be specified in the YAML metadata file (`.yml`), where pixel format,
resolution and frame stride shall be properly specified.

```txt
usage: sds-convert.py video [-h]
                            -i <input_file>
                            -o <output_file>
                            -y <yaml_file>

Convert SDS file to MP4 video file

options:
  -h, --help        show this help message and exit

required:
  -i <input_file>   Input file
  -o <output_file>  Output file
  -y <yaml_file>    YAML metadata file
```

!!! Note
    - The tool expects the SDS file to contain strictly video frames - no header markers or custom formatting.

**Example of metadata yml file for RGB888 video stream:**

```yml
sds:
  name: Video Stream - RGB888
  description: 192 x 192 RGB888 video frames
  frequency: 30
  content:
    - value: Frame
      type: uint8_t
      image:
        pixel_format: RGB888
        width: 192
        height: 192
        stride_bytes: 576   # 3 bytes per pixel
```

**Example:**

```bash
python sds-convert.py video -i Camera.0.sds -o Camera.mp4 -y Camera.sds.yml
```

!!! Note
    - [Theory of Operation - Image Metadata Format](theory.md#image-metadata-format) contains more information about the supported video formats.

## SDS-Check

The Python utility [**SDS-Check**](https://github.com/ARM-software/SDS-Framework/tree/main/utilities) checks SDS files for correctness and consistency.

The following checks are performed:

- [Size consistency check](#size-consistency-check): Checks that data size of all records matches the size of the SDS file.
- [Timeslot consistency check](#timeslot-consistency-check): Verifies that the timeslots of the records are in ascending order.
- [Jitter check](#jitter-check): Searches for the record with the largest deviation from the average timeslot interval and prints its record number and deviation.
- [Delta time check](#delta-time-check): Checks for the record with the largest timeslot difference compared to the following record.
- [Duplicate timeslot check](#duplicate-timeslot-check): Checks for records with identical timeslots.

### Usage

- [Setup](#setup) the Python environment.
- Invoke the tool as explained below.

```txt
usage: sds-check.py [-h] -i <sds_file> [-t <tick_rate>]

SDS data validation

options:
  -h, --help      Show this help message and exit

required:
  -i <sds_file>   SDS file

optional:
  -t <tick_rate>  Timeslot tick rate in Hz (default: 1000 for 1 ms tick interval)
```

**Example:**

```txt
python sds-check.py -i Accelerometer.0.sds
File Name         : Accelerometer.0.sds
File Size         : 156.020 bytes
Number of Records : 289
Recording Time    : 14 s
Recording Interval: 50 ms
Data Size         : 153.748 bytes
Block Size        : 532 bytes
Data Rate         : 10.640 byte/s
Jitter            : Not detected
Validation passed
```

!!! Note
    The default tick frequency is 1000 Hz.

### Summary Report

After processing the SDS file, the SDS-Check utility prints a summary report with the following information:

- **File Name**         : Name of the SDS file
- **File Size**         : Size of the file in bytes
- **Number of Records** : Total number of records
- **Recording Time**    : Duration of the recording in seconds or milliseconds
- **Recording Interval**: Time interval between records in milliseconds
- **Data Size**         : Total size of the data without record headers in bytes
- **Block Size**        : Data block size, if all data blocks are of the same size
- **Max Block Size**    : Maximum data block size, if different from the average data block size
- **Min Block Size**    : Minimum data block size, if different from the average data block size
- **Avg Block Size**    : Average data block size
- **Data Rate**         : Recorded data rate in bytes per second
- **Max Jitter**        : Maximum deviation from the expected timeslots, if detected (optional)
- **Max Delta Time**    : Largest difference of the neighboring timeslots, if deviating from the recording interval (optional)
- **Duplicate Tslots**  : Number of duplicated timeslots, if found

### Size consistency check

This check processes the SDS data records and calculates the total size of the SDS data.
It is the sum of all data records (header + data). This data size should match the size of
the SDS file. If the sizes do not match, an error such as the one below is printed:

```txt
Error: File size mismatch. Expected 360 bytes, but file contains 363 bytes.
```

### Timeslot consistency check

This check processes the SDS records and ensures that the timeslots recorded in the records
are arranged in ascending order. If the timeslots are not in the ascending order, an error
such as the one below is printed:

```txt
Error: Timeslot not in ascending order in record 23.
```

### Jitter check

This check processes the SDS data records and searches for the maximum deviation of the recorded
timeslots from the expected ones. If a deviation is found, the maximum deviation is
evaluated as **jitter** and, together with the record number, is printed in the summary report.

```txt
File Name         : Gyroscope.0.sds
  :
Max Jitter        : 59 ms, in record 19
Validation passed
```

### Delta time check

This check processes the SDS records and finds the largest difference in timeslots
between two neighboring records, called **Max Delta Time**.

Normally, in SDS files, the delta time and the recording interval are identical, so no information
about the delta time status is printed. If the delta time and the recording interval are not identical,
that is, a difference is detected, the **Max Delta Time** together with the record number is printed
in the summary report.

```txt
File Name         : Temperature.0.sds
  :
Max Delta Time    : 1.050 ms, in record 2
Validation passed
```

This is not an error, but a report of an anomaly. If the time delta is significantly larger than the
sampling interval, for example many times longer, this may indicate that one or more data records are
missing from the SDS file.

### Duplicate timeslot check

This check processes the SDS records to identify duplicated timeslots. This means that the same timeslot
is used in several consecutive data records.

This may indicate that the recording loop in an embedded application is not set up correctly. It is also
possible that duplicate timeslots are caused by unexpected thread delays in the embedded application.

Duplicate timeslots are unusual in typical recording files. If multiple timeslots with the same value
are found in the SDS file, **Duplicate Tslots** will be added to the summary report.

```txt
File Name         : DataInput.0.sds
  :
Duplicate Tslots  : 4, found at record 1
Validation passed
```

This is not an error, but a report of an anomaly. The report contains the number of records with
the same timeslot and the position in the SDS file where the anomaly was detected (record number).

!!! Note
    Only the first occurrence of a duplicate timeslot is reported.
