# SDS Player Example

This project shows how to use **Synchronous Data Stream Player** via File System (Semihosting). The application uses SDS Player to read pre-recorded accelerometer data, which is processed on the fly to determine if any movement is detected.

Available targets:
 - AVH_MPS3_Corstone-300: runs on Arm Virtual Hardware (AVH) for MPS3 platform with Corstone-300
   >Note: AVH reads accelerometer data from files on the host PC (previously recorded)

## Prerequisites

### Tools:
 - [CMSIS-Toolbox 2.0.0](https://github.com/Open-CMSIS-Pack/cmsis-toolbox/releases/) or later
 - Arm Compiler 6.18 or later
 - [python 3.9 or later](https://www.python.org/downloads/windows/) when using Arm Virtual Hardware (AVH)

## Compile Project

The following commands convert and build the project with build type `Debug` and target type `AVH_MPS3_Corstone-300`:
```sh
cbuild SDS_Player.csolution.yml --update-rte -p --context .Debug+AVH_MPS3_Corstone-300
```

## Execute

### AVH_MPS3_Corstone-300 target

- copy recorded accelerometer files (Accelerometer.x.sds) from [recordings folder](../recordings) to [project folder](./)
- from the project folder execute following command to run the example on the AVH simulation model:
  ```sh
  FVP_Corstone_SSE-300 -f ../framework/layer/Board/AVH_MPS3_Corstone-300/fvp_config.txt out/SDS_Player/AVH_MPS3_Corstone-300/Debug/SDS_Player.axf
  ```
- monitor results (if motion detected) in terminal window
