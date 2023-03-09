# SDS-Recorder Integration Example "QeeXo AutoML"

This guide provides step-by-step instructions on how to set up and integrate the SDS-Recorder on the B-U585I-IOT02A Discovery kit, record data using the SDS-Socket, export and train a model on qeexo AutoML platform, and integrate the exported library into a demo application.

## Prerequisites

Before proceeding with the SDS-Recorder setup and integration, make sure you have the following:

- STM32U5 board with STM32U5x2 MCU https://www.st.com/en/evaluation-tools/b-u585i-iot02a.html
- This repository cloned on your computer https://github.com/MatthiasHertel80/SDS-Framework
- Python 3.9 or later https://www.python.org/downloads/windows/
- Qeexo AutoML account https://automl.qeexo.com/
- Keil MDK v5.35 or later https://www.keil.com/demo/eval/arm.htm
- Basic knowledge of C programming, terminal, and command line tools

## Step 1: Build the SDS-Recorder on B-U585I-IOT02A Discovery kit board

To build the SDS-Recorder on the B-U585I-IOT02A Discovery kit using Keil MDK, follow these steps:

- Clone the SDS-Framework repository from GitHub. 
- Connect the STM32U5 board to your computer via USB.
- Open Demo.Debug+HW.uvprojx in Keil MDK. 
- Configure Host PC IP Address:
   - Modify the following definitions in [sdsio_socket.c](../sds/source/sdsio_socket.c):
   - `SERVER_IP`: IP address of the host PC running python script (`SDS-Socket.py`)
- Configure WiFi Access Point:
   - Modify the following definitions in [socket_startup.c](Socket/WiFi/socket_startup.c):
   - `SSID`:          WiFi Access Point SSID
   - `PASSWORD`:      WiFi Access Point Password
   - `SECURITY_TYPE`: WiFi Access Point Security


- Once the build is complete, flash the firmware to the STM32U5 board using the Flash button from the toolbar.

## Step 2: Run the SDS-Socket and record data

To record data using the SDS-Socket application, follow these steps:

 - run `python ../utilities/SDS-Socket/SDS-Socket.py` to start the socket server on the host PC
 - connect the board's ST-Link USB to the PC (provides also power)
 - open terminal on the PC and connect to the board's serial port (Baud rate: 115200)
 - reset the target (press RST button on the board)
 - wait until connected to WiFi (status printed to the terminal)
 - press USER (vioBUTTON0) button to start/stop reading and recording sensor data
   >Note: The recordings start with index 0 and increment for each subsequent start/stop.

Sensor data is recorded to files `<sensor_name>.<index>.sds` and also printed to the terminal.

For the subsequent step start recording data for at least 10 seconds for each activity.

## Step 3: Export and train model on qeexo AutoML platform

To export and train a model on the qeexo AutoML platform, follow these steps:

Todo

## Step 4: Integrate exported library into the application, build and run.

To integrate the exported library into the application, build and run the SDS-Recorder firmware, follow these steps:

1. Copy the exported library into the ./demoapp/qeexo folder. The folder contains a pretrained model for a SHAKE and WAVE gesture classification. You can overwrite this.
2. Open Demo.Debug+HW.uvprojx in Keil MDK. 
3. Build and run the application, using the uVision debugger.

## Conclusion

Following the above steps will allow you to set up and integrate the SDS-Recorder on the STM32U5 board, record data using the SDS-Socket application, export and train a model on the qeexo AutoML platform, and integrate the exported library into the application.
