# SDS Recorder Example

This project shows how to use **Synchronous Data Stream Recorder** via File System (MDK FS). The application reads sensors and records the sensor data to a file on the SD Card. Push button (vioBUTTON0) starts and stops the operations.

Available target:
  - IMXRT1050-EVKB: runs on NXP IMXRT1050-EVKB board

## Prerequisites

### Hardware:
 - [IMXRT1050-EVKB](https://www.nxp.com/design/development-boards/i-mx-evaluation-and-development-boards/i-mx-rt1050-evaluation-kit:MIMXRT1050-EVK) from NXP

### Tools:
 - [CMSIS-Toolbox 2.0.0](https://github.com/Open-CMSIS-Pack/cmsis-toolbox/releases/) or later
 - Arm Compiler 6.18 or later

## Compile Project

The following commands convert and build the project with build type `Debug` and target type `IMXRT1050-EVKB`:

```sh
cbuild SDS_Recorder.csolution.yml --update-rte -p --context .Debug+IMXRT1050-EVKB
```

## Execute

### IMXRT1050-EVKB target
 - insert SD Card into a on-board SD Card slot
 - connect the board's DAPLink USB to the PC (provides also power)
 - use a programmer to download the HW image to the hardware target
 - open terminal on the PC and connect to the board's serial port (Baud rate: 115200)
 - reset the target (press RST button on the board)
 - press USER (vioBUTTON0) button to start/stop reading and recording sensor data
   >Note: The recordings start with index 0 and increment for each subsequent start/stop.
 - sensor data is recorded to files `<sensor>.<index>.sds` on SD Card.
