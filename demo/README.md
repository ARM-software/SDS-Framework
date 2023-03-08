# SDS Demo

This project is a simple demo for the **Synchronous Data Stream (SDS) Framework**.

The application reads sensors, optionally records the sensor data and prints the sensor data to a terminal.
Push button (vioBUTTON0) starts and stops the operations.

Demo is available for the following targets:
 - HW: runs on B-U585I-IOT02A board with on-board WiFi module
   - reads actual sensor data from hardware
   - records sensor data via socket (using WiFi interface) to files on the host PC
   - prints sensor data to a terminal
 - AVH: runs on Arm Virtual Hardware (AVH) for MPS3 platform with Corstone-300
   - reads sensor data from files on the host PC (previously recorded)
   - prints sensor data to a terminal

## Prerequisites

### Hardware:
 - [B-U585I-IOT02A Discovery kit](https://www.st.com/en/evaluation-tools/b-u585i-iot02a.html) from STMicroelectronics
 - WiFi Access Point (board and PC connected to the same local network)

### Software:
 - [CMSIS-Toolbox 1.5.0](https://github.com/Open-CMSIS-Pack/cmsis-toolbox/releases/tag/1.5.0) or later
 - Arm Compiler 6.18 or later
 - [python 3.9 or later](https://www.python.org/downloads/windows/)

### CMSIS Packs:
 - Required packs are listed in the file [`Demo.csolution.yml`](./Demo.csolution.yml)  
   Packs can be installed by executing the following `csolution` and `cpackget` commands:
   ```
   csolution list packs -s Demo.csolution.yml -m >packs.txt
   cpackget add -f packs.txt
   ```

## Configure

Host PC IP Address:
 - Modify the following definitions in [sdsio_socket.c](../sds/source/sdsio_socket.c):
   - `SERVER_IP`: IP address of the host PC running python script (`SDS-Socket.py`)

WiFi Access Point:
 - Modify the following definitions in [socket_startup.c](Socket/WiFi/socket_startup.c):
   - `SSID`:          WiFi Access Point SSID
   - `PASSWORD`:      WiFi Access Point Password
   - `SECURITY_TYPE`: WiFi Access Point Security

## Build

1. Use the `csolution` command to create `.cprj` project files.
   ```
   > csolution convert -s Demo.csolution.yml
   ```

2. Use the `cbuild` command to create executable files.
   ```
   > cbuild Demo.Debug+HW.cprj
   > cbuild Demo.Debug+AVH.cprj
   ```

## Program

Use a programmer to download the HW image to the hardware target.

## Run

### HW Target (B-U585I-IOT02A)

Execute the following steps:
 - run [SDSIO-Server](../utilities/SDSIO-Server/README.md) to start the SDS I/O server on the host PC
 - connect the board's ST-Link USB to the PC (provides also power)
 - open terminal on the PC and connect to the board's serial port (Baud rate: 115200)
 - reset the target (press RST button on the board)
 - wait until connected to WiFi (status printed to the terminal)
 - press USER (vioBUTTON0) button to start/stop reading and recording sensor data
   >Note: The recordings start with index 0 and increment for each subsequent start/stop.

Sensor data is recorded to files `<sensor_name>.<index>.sds` and also printed to the terminal.

### AVH Target

Execute the following steps:
 - run the VHT model from the command line by executing:
   ```
   VHT_MPS3_Corstone_SSE-300 -f Board/AVH_MPS3_Corstone-300/fvp_config.txt -V ../sensor/vsi/python out/Demo/AVH/Debug/Debug+AVH.axf
   ```
 - press PB0 (vioBUTTON0) button (double click PB0 in the VHT visualization dialog)
   to start/stop reading sensor data

Sensor data is read from files `<sensor_name>.<index>.sds` and also printed to the terminal.
