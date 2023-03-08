# SDSIO-Server
Python based SDS I/O Server for PC.

It captures SDS recorder data sent from the target via one of the supported interfaces and writes recordings to files on the host.

Sensor data is recorded to files `<sensor_name>.<index>.sds`:
 - `<sensor_name>` is the sensor name specified from the target
 - `<index>` is the zero-based index which is incremented for each subsequent recording

## Supported interfaces
- **socket**  
   SDS recorder data is sent from the target via TCP socket. Works together with the matching implementation on the target ([sdsio_socket.c](../../sds/source/sdsio_socket.c)).

- **serial**  
   SDS recorder data is sent from the target via serial port. Works together with the matching implementation on the target ([sdsio_vcom.c](../../sds/source/sdsio_vcom.c)).

## Set-up and requirements
### Requirements
- Python 3.9 or later with packages:
  - pyserial

### Set-up
1. Open terminal in SDSIO-Server root folder
2. Check installed Python version with:
   ```
   python --version
   ```
3. (Optional) Use Python environment
   1. Create Python environment:
      ```
      python -m venv <env_name>
      ```
      >Note: Usually **`env`** is used for `<env_name>`
   2. Activate created Python environment:
      ```
      <env_name>/Scripts/activate
      ```
4. Install required Python packages:
   ```
   pip install pyserial
   ```

## Usage
Print help (common) with:
```
python sdsio-server.py --help
```

```
usage: sdsio-server.py [-h] {socket,serial} ...

SDS I/O server

positional arguments:
  {socket,serial}

options:
  -h, --help       show this help message and exit
```


### Socket
Print help (socket) with:
```
python sdsio-server.py socket --help
```

```
usage: sdsio-server.py socket [-h] [--port <TCP Port>] [--outdir <Output dir>]

options:
  -h, --help             show this help message and exit

optional:
  --port <TCP Port>      TCP port (default: 5050)
  --outdir <Output dir>  Output directory
```


### Serial
Print help (serial) with:
```
python sdsio-server.py serial --help
```

```
usage: sdsio-server.py serial [-h] -p <Serial Port> [--baudrate <Baudrate>] [--parity <Parity>] [--stopbits <Stop bits>] [--outdir <Output dir>]

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

### Examples
- Socket:
   ```
   python sdsio-server.py socket --outdir ./out_dir
   ```
- Serial:
   ```
   python sdsio-server.py serial -p COM0 --baudrate 115200 --outdir ./out_dir
   ```

>Note: The server is stopped by Ctrl + C
