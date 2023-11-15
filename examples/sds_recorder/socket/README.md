# SDS Recorder Example

This project shows how to use **Synchronous Data Stream Recorder** via Socket (IoT Socket). The application reads sensors and records the sensor data to a file on the host PC. Push button (vioBUTTON0) starts and stops the operations.

Available targets:
 - B-U585I-IOT02A: runs on STMicroelectronics B-U585I-IOT02A board
 - AVH_MPS3_Corstone-300: runs on Arm Virtual Hardware (AVH) for MPS3 platform with Corstone-300
   >Note: AVH reads accelerometer data from files on the host PC (previously recorded)

## Prerequisites

### Hardware:
 - [B-U585I-IOT02A Discovery kit](https://www.st.com/en/evaluation-tools/b-u585i-iot02a.html) from STMicroelectronics

### Tools:
 - [CMSIS-Toolbox 2.0.0](https://github.com/Open-CMSIS-Pack/cmsis-toolbox/releases/) or later
 - Arm Compiler 6.18 or later
 - [python 3.9 or later](https://www.python.org/downloads/windows/)

### Configure:

Host PC IP Address:
 - Modify the following definitions in [sdsio_socket.c](./RTE/SDS/sdsio_config_socket.h):
   - `SERVER_IP`: IP address of the host PC running python script (`SDSIO-Server.py`)

WiFi Access Point:
 - Modify the following definitions in [socket_startup.c](../../framework/layer/Socket/WiFi/socket_startup.c):
   - `SSID`:          WiFi Access Point SSID
   - `PASSWORD`:      WiFi Access Point Password
   - `SECURITY_TYPE`: WiFi Access Point Security

## Compile Project

The following commands convert and build the project with build type `Debug` and target type `B-U585I-IOT02A`:

```sh
cbuild SDS_Recorder.csolution.yml --update-rte -p --context .Debug+B-U585I-IOT02A
```

## Execute

### B-U585I-IOT02A target
 - connect the board's ST-Link USB to the PC (provides also power)
 - use a programmer to download the HW image to the hardware target
 - run [SDSIO-Server](../../../utilities/SDSIO-Server/README.md) to start the SDS I/O server on the host PC
 - open terminal on the PC and connect to the board's serial port (Baud rate: 115200)
 - reset the target (press RST button on the board)
 - wait until connected to WiFi (status printed to the terminal)
 - press USER (vioBUTTON0) button to start/stop reading and recording sensor data
   >Note: The recordings start with index 0 and increment for each subsequent start/stop.
 - sensor data is recorded to files `<sensor>.<index>.sds` on the host PC.

 ### AVH_MPS3_Corstone-300 Target
- copy recorded sensor files (Accelerometer.x.sds, Gyroscope.x.sds and Temperature.x.sds ) from [recordings folder](../recordings) to [project folder](./)
- from the project folder execute following command to run the example on the VHT simulation model:

```sh
VHT_MPS3_Corstone_SSE-300 -f ../../framework/layer/Board/AVH_MPS3_Corstone-300/fvp_config.txt -V ../../framework/interface/VSI/sensor/python out/SDS_Recorder/AVH_MPS3_Corstone-300/Debug/SDS_Recorder.axf
```
 - run [SDSIO-Server](../../../utilities/SDSIO-Server/README.md) to start the SDS I/O server on the host PC
 - press PB0 (vioBUTTON0) button (double click PB0 in the VHT visualization dialog)
   to start/stop reading sensor data
   >Note: The recordings start with index 0 and increment for each subsequent start/stop.
 - sensor data is recorded to files `<sensor>.<index>.sds` on the host PC.
