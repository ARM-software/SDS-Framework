# SDS Recorder Example

This project shows how to use **Synchronous Data Stream Recorder** via USB Virtual COM Port (MDK USB). The application reads sensors and records the sensor data to a file on the host PC. Push button (vioBUTTON0) starts and stops the operations.

Available target:
 - B-U585I-IOT02A: runs on STMicroelectronics B-U585I-IOT02A board

## Prerequisites

### Hardware:
 - [B-U585I-IOT02A Discovery kit](https://www.st.com/en/evaluation-tools/b-u585i-iot02a.html) from STMicroelectronics

### Tools:
 - [CMSIS-Toolbox 2.0.0](https://github.com/Open-CMSIS-Pack/cmsis-toolbox/releases/) or later
 - Arm Compiler 6.18 or later
 - [python 3.9 or later](https://www.python.org/downloads/windows/)

## Compile Project

The following commands convert and build the project with build type `Debug` and target type `B-U585I-IOT02A`:

```sh
cbuild SDS_Recorder.csolution.yml --update-rte -p --context .Debug+B-U585I-IOT02A
```

## Execute

### B-U585I-IOT02A target
 - connect the board's ST-Link USB to the PC (provides also power)
 - use a programmer to download the HW image to the hardware target
 - connect the board's USB (CN1 USB-C) to host PC
 - run [SDSIO-Server](../../../../utilities/SDSIO-Server/README.md) to start the SDS I/O server on the host PC
 - open terminal on the PC and connect to the board's serial port (Baud rate: 115200)
 - reset the target (press RST button on the board)
 - press USER (vioBUTTON0) button to start/stop reading and recording sensor data
   >Note: The recordings start with index 0 and increment for each subsequent start/stop.
 - sensor data is recorded to files `<sensor>.<index>.sds` on the host PC.
