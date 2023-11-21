# SDS Buffer Example

This project shows how to use **Synchronous Data Stream Recorder Buffer** (Non-Blocking Read/Write to circular buffer). Application has two user threads. First thread is reading the data from the accelerometer sensor and writing the data to the SDS circular buffer. When buffer threshold is reached, second thread is notified that data is available. Second thread then reads the data from the SDS circular buffer, processes the data to detect the movement and prints the result.

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
 - [python 3.9 or later](https://www.python.org/downloads/windows/) when using Arm Virtual Hardware (AVH)

## Compile Project

The following commands convert and build the project with build type `Debug` and target type `B-U585I-IOT02A`:

```sh
cbuild SDS_Buffer.csolution.yml --update-rte -p --context .Debug+B-U585I-IOT02A
```

## Execute

### B-U585I-IOT02A target
- connect the board's ST-Link USB to the PC (provides also power)
- use a programmer to download the HW image to the hardware target
- open terminal on the PC and connect to the board's serial port (Baud rate: 115200)
- reset the target (press RST button on the board)
- monitor results (if motion detected) in terminal window

### AVH_MPS3_Corstone-300 target

- copy recorded accelerometer files (Accelerometer.x.sds) from [recordings folder](../recordings) to [project folder](./)
- from the project folder execute following command to run the example for 60 seconds on the AVH simulation model:

```sh
FVP_Corstone_SSE-300 -f ../framework/layer/Board/AVH_MPS3_Corstone-300/fvp_config.txt -C mps3_board.v_path=../framework/interface/VSI/sensor/python --simlimit=60 out/SDS_Buffer/AVH_MPS3_Corstone-300/Debug/SDS_Buffer.axf
```
- monitor results (if motion detected) in terminal window
