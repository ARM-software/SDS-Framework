# SDS Recorder Example

This project shows how to use **Synchronous Data Stream Recorder** via File System (Semihosting). The application reads sensors and records the sensor data to a file on the Host PC. Push button (vioBUTTON0) starts and stops the operations.

Available target:
 - AVH_MPS3_Corstone-300: runs on Arm Virtual Hardware (AVH) for MPS3 platform with Corstone-300
   >Note: AVH reads accelerometer data from files on the host PC (previously recorded)

## Prerequisites

### Tools:
 - [CMSIS-Toolbox 2.0.0](https://github.com/Open-CMSIS-Pack/cmsis-toolbox/releases/) or later
 - Arm Compiler 6.18 or later
 - [python 3.9 or later](https://www.python.org/downloads/windows/)

## Compile Project

The following commands convert and build the project with build type `Debug` and target type `AVH_MPS3_Corstone-300`:

```sh
cbuild SDS_Recorder.csolution.yml --update-rte -p --configuration .Debug+AVH_MPS3_Corstone-300
```

## Execute

### AVH_MPS3_Corstone-300 Target
- copy recorded sensor files (Accelerometer.x.sds, Gyroscope.x.sds and Temperature.x.sds ) from [recordings folder](../recordings) to [project folder](./)
- from the project folder execute following command to run the example on the VHT simulation model:

```sh
VHT_MPS3_Corstone_SSE-300 -f ../../../framework/layer/Board/AVH_MPS3_Corstone-300/fvp_config.txt -V ../../../framework/interface/VSI/sensor/python out/SDS_Recorder/AVH_MPS3_Corstone-300/Debug/SDS_Recorder.axf
```
 - press PB0 (vioBUTTON0) button (double click PB0 in the VHT visualization dialog)
   to start/stop reading sensor data
   >Note: The recordings start with index 0 and increment for each subsequent start/stop.
 - sensor data is recorded to files `<sensor>.<index>.sds` on the host PC.
