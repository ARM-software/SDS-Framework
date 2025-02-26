# Utilities

<!-- markdownlint-disable MD013 -->
<!-- markdownlint-disable MD036 -->
<!-- markdownlint-disable MD024 -->

The SDS-Framework includes the following utilities that are implemented in Python.

- [**SDSIO-Server:**](#sdsio-server) enables recording and playback of SDS data files via socket (TCP/IP) or serial (UART) connection.
- [**SDS-View:**](#sds-view) graphical data viewer for SDS data files.
- [**SDS-Convert:**](#sds-convert) convert SDS data files into CSV, Qeexo V2 CSV, or WAV format.

## SDSIO-Server

The Python utility [**SDSIO-Server**](https://github.com/ARM-software/SDS-Framework/tree/main/utilities/SDSIO-Server) enables recording and playback of SDS data files via socket (TCP/IP) or serial (UART) connection.
It communicates with the target using these [SDSIO interfaces](https://github.com/ARM-software/SDS-Framework/tree/main/sds/source/sdsio):

- [serial/usart](https://github.com/ARM-software/SDS-Framework/tree/main/sds/source/sdsio/serial/usart) for serial communication via CMSIS-Driver USART.
- [socket](https://github.com/ARM-software/SDS-Framework/tree/main/sds/source/sdsio/socket) for TCP/IP communication using MDK-Middleware, LwIP, or CMSIS-Driver WiFi.
- [vcom/mdk](https://github.com/ARM-software/SDS-Framework/tree/main/sds/source/sdsio/vcom/mdk) for serial communication via USB VCom using MDK-Middleware.

The SDS data stream is recorded to files with the following naming convention:

`<name>.<index>.sds`

- `<name>` is the name of the I/O stream specified with the function `sdsRecOpen` or `sdsPlayOpen` on the target.
- `<index>` is the zero-based index which is incremented for each subsequent recording.

The data content of the `<name>.<index>.sds` is described with metadata file `<name>.sds.yml` in [YAML format](https://github.com/ARM-software/SDS-Framework/tree/main/schema).

### Usage

- Setup the Python-based `sdsio-server.py` tool as described in [utilities/SDSIO-Server/README.md](https://github.com/ARM-software/SDS-Framework/edit/main/utilities/SDSIO-Server/README.md).
- Depending on the SDS interface used on the target use either [Socket Mode](#socket-mode) or [Serial Mode](#serial-mode) as described below.
- The SDSIO_Server terminates with `Ctrl+C`.

#### Socket Mode

```txt
usage: sdsio-server.py socket [-h] [--ipaddr <IP> | --interface <Interface>] [--port <TCP Port>]
                              [--outdir <Output dir>]

options:
  -h, --help               show this help message and exit

optional:
  --ipaddr <IP>            Server IP address (not allowed with argument --interface)
  --interface <Interface>  Network interface (not allowed with argument --ipaddr)
  --port <TCP Port>        TCP port (default: 5050)
  --outdir <Output dir>    Output directory
```

**Example:**

```bash
python sdsio-server.py socket --interface eth0 --outdir ./out_dir
```

#### Serial Mode

```txt
usage: sdsio-server.py serial [-h] -p <Serial Port> [--baudrate <Baudrate>] [--parity <Parity>] 
                              [--stopbits <Stop bits>] [--outdir <Output dir>]

options:
  -h, --help              show this help message and exit

required:
  -p <Serial Port>        Serial port

optional:
  --baudrate <Baudrate>   Baudrate (default: 115200)
  --parity <Parity>       Parity: N = None, E = Even, O = Odd, M = Mark, S = Space (default: N)
  --stopbits <Stop bits>  Stop bits: 1, 1.5, 2 (default: 1)
  --outdir <Output dir>   Output directory
```

**Example:**

```bash
python sdsio-server.py serial -p COM0 --baudrate 115200 --outdir ./out_dir
```

## SDS-View

The Python utility [**SDSIO-View**](https://github.com/ARM-software/SDS-Framework/tree/main/utilities/SDS-View) outputs a time-based plot of SDS data files (`<name>.<index>.sds`) based on the meta-data file (`<name>.sds.yml`.

The horizontal time scale is derived from the number of data points in a recording and frequency provided in the metadata description. All plots form a single recording will be displayed on the same figure (shared vertical scale).

If there are 3 values described in the metadata file, an optional 3D view may be displayed.  

### Usage

- Setup the Python-based `sds-view.py` tool as described in [utilities/SDS-View/README.md](https://github.com/ARM-software/SDS-Framework/edit/main/utilities/SDS-View/README.md).
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

## SDS-Convert

The Python utility [**SDSIO-Convert**](https://github.com/ARM-software/SDS-Framework/tree/main/utilities/SDS-Convert) converts SDS data files to selected format based on description in metadata (YAML) files.  

### Usage

- Setup the Python-based `sds-convert.py` tool as described in [utilities/SDS-Convert/README.md](https://github.com/ARM-software/SDS-Framework/edit/main/utilities/SDS-Convert/README.md).
- Depending on the required format use the tool as shown below.

#### Audio WAV

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
    The metadata and SDS data file pairs must be passed as arguments in the same order to decoded data correctly.

**Example:**

```bash
python sds-convert.py audio_wav -i Microphone.0.sds -o microphone.wav -y Microphone.sds.yml
```

#### Simple CSV

```txt
usage: sds-convert.py simple_csv [-h] -i <input_file> [<input_file> ...] -o <output_file> [-y <yaml_file> [<yaml_file> ...]] [--normalize] [--start-tick <start-tick>] [--stop-tick <stop-tick>]

options:
  -h, --help                          show this help message and exit

required:
  -i <input_file> [<input_file> ...]  Input file
  -o <output_file>                    Output file

optional:
  -y <yaml_file> [<yaml_file> ...]    YAML sensor description file
  --normalize                         Normalize timestamps so they start with 0
  --start-tick <start-tick>           Exported data start tick (default: None)
  --stop-tick <stop-tick>             Exported data stop tick (default: None)
```

!!! Note
    The metadata and SDS data file pairs must be passed as arguments in the same order to decoded data correctly.

**Example:**

```bash
python sds-convert.py simple_csv  -i Gyroscope.0.sds -o gyroscope_simple.csv -y Gyroscope.sds.yml --normalize --start-tick 0.2 --stop-tick 0.3
```

#### Qeexo V2 CSV

```txt
usage: sds-convert.py qeexo_v2_csv [-h] -i <input_file> [<input_file> ...] -o <output_file> [-y <yaml_file> [<yaml_file> ...]] [--normalize] [--start-tick <start-tick>] [--stop-tick <stop-tick>] [--label 'label'] [--interval <interval>] [--sds_index <sds_index>]

options:
  -h, --help                          show this help message and exit

required:
  -i <input_file> [<input_file> ...]  Input file
  -o <output_file>                    Output file

optional:
  -y <yaml_file> [<yaml_file> ...]    YAML sensor description file
  --normalize                         Normalize timestamps so they start with 0
  --start-tick <start-tick>           Exported data start tick (default: None)
  --stop-tick <stop-tick>             Exported data stop tick (default: None)
  --label 'label'                     Qeexo class label for sensor data (default: None)
  --interval <interval>               Qeexo timestamp interval in ms (default: 50)
  --sds_index <sds_index>             SDS file index to write (default: <sensor>.0.sds)
```

!!! Note
    The metadata and SDS data file pairs must be passed as arguments in the same order to decoded data correctly.

**Examples:**

Convert SDS data files to Qeexo V2 CSV files:

```bash
python sds-convert.py qeexo_v2_csv -i Gyroscope.0.sds Accelerometer.0.sds -o sensor_fusion.csv -y Gyroscope.sds.yaml Accelerometer.sds.yaml --normalize --start-tick 200 --stop-tick 300
```

Convert Qeexo V2 CSV files to SDS data files:

```bash
python sds-convert qeexo_v2_csv -i accelerometer_data.csv -o accelerometer.sds
```

