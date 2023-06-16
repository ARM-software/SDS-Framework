Board: STMicroelectronics [B-U585I-IOT02A](https://www.st.com/en/evaluation-tools/b-u585i-iot02a.html)
------------------------------------------

Device: **STM32U585AII6QU**  
System Core Clock: **160 MHz**

This setup is configured using **STM32CubeMX**, an interactive tool provided by STMicroelectronics for device configuration.
Refer to ["Create Projects with STM32Cube HAL and STM32CubeMX"](https://www.keil.com/pack/doc/STM32Cube/html/index.html) for additional information.

For **STM32CubeMX** configuration settings please refer to [STM32CubeMX Configuration](STM32CubeMX/STM32CubeMX.pdf).

The Heap/stack settings and CMSIS-Driver assignments are configured in the configuration files of respective software components.

The example project can be re-configured to work on custom hardware.
Refer to ["Migrate STM32 Based Example Projects to Custom Hardware"](https://github.com/MDK-Packs/Documentation/tree/master/Porting_to_Custom_Hardware) for additional information.

### System Configuration

**TrustZone** disabled (option bit TZEN=0)

| System resource         | Setting
|:------------------------|:--------------------------------------------
| Heap                    | 64 kB (configured in the linker script file)
| Stack (MSP)             | 1 kB (configured in the linker script file)

### STDIO mapping

**STDIO** is routed to Virtual COM port on the ST-Link (using USART1 peripheral)

### CMSIS-Driver mapping

| CMSIS-Driver  | Peripheral  | Physical connection
|:--------------|:------------|:------------------------------------
| Driver_SPI1   | SPI1        | Arduino UNO R3 connector (CN13)
| Driver_USART3 | USART3      | Arduino UNO R3 connector (CN13)
| Driver_USART4 | UART4       | Bluetooth Low Energy Wireless module
| Driver_WiFi0  | SPI2        | WiFi MXCHIP EMW3080 module
| Driver_GPIO0  | GPIO0       | Digital I/O pins

### CMSIS-Driver Virtual I/O mapping

| CMSIS-Driver VIO  | Physical resource
|:------------------|:-------------------------------
| vioBUTTON0        | Button USER (PC13)
| vioLED0           | LED RED (PH6)
| vioLED1           | LED GREEN (PH7)
| vioMotionGyro     | iNEMO 3D gyroscope (ISM330DLC)
| vioMotionAccelero | iNEMO 3D accelerometer (ISM330DLC)
| vioMotionMagneto  | High accuracy 3-axis magnetometer (IIS2MDC)

### Arduino UNO mapping

| Arduino resource  | Driver
|:------------------|:-------------------------------
| UART (D0,D1)      | USART3 Driver (ARDUINO_UNO_UART)
| SPI (D11,D12,D13) | SPI1 Driver (ARDUINO_UNO_SPI)
| I2C (D20,D21)     | I2C1 Driver (ARDUINO_UNO_I2C)
| Digital I/O: D2   | GPIO0 Driver (ARDUINO_UNO_D2)
| Digital I/O: D3   | GPIO0 Driver (ARDUINO_UNO_D3)
| Digital I/O: D4   | GPIO0 Driver (ARDUINO_UNO_D4)
| Digital I/O: D5   | GPIO0 Driver (ARDUINO_UNO_D5)
| Digital I/O: D6   | GPIO0 Driver (ARDUINO_UNO_D6)
| Digital I/O: D7   | GPIO0 Driver (ARDUINO_UNO_D7)
| Digital I/O: D8   | GPIO0 Driver (ARDUINO_UNO_D8)
| Digital I/O: D9   | GPIO0 Driver (ARDUINO_UNO_D9)
| Digital I/O: D10  | GPIO0 Driver (ARDUINO_UNO_D10)
| Digital I/O: D14  | GPIO0 Driver (ARDUINO_UNO_D14)
| Digital I/O: D15  | GPIO0 Driver (ARDUINO_UNO_D15)
| Digital I/O: D16  | GPIO0 Driver (ARDUINO_UNO_D16)
| Digital I/O: D17  | GPIO0 Driver (ARDUINO_UNO_D17)
| Digital I/O: D18  | GPIO0 Driver (ARDUINO_UNO_D18)
| Digital I/O: D19  | GPIO0 Driver (ARDUINO_UNO_D19)

## Board configuration

**Board setup**

  - Insure that the **5V_USB_STLK** and **JP3** jumpers are bridged and the remaining jumpers are not bridged
  - Check that the **BOOT0** DIP switch is in the **0** / right position (closest to the ST-LINK STLK CN8 USB connector)
  - Connect a **USB micro-B cable** between the **STLK** connector and your **Personal Computer**

**WiFi module firmware update** (required for older board revision C01)

  - Download the [EMW3080 update tool](https://www.st.com/content/ccc/resource/technical/software/firmware/group1/48/a2/e8/27/7f/ae/4b/26/x-wifi-emw3080b/files/x-wifi-emw3080b.zip/jcr:content/translations/en.x-wifi-emw3080b.zip) archive from the STMicroelectronics website
  - Extract the downloaded archive
  - Connect a **USB micro-B cable** between the **STLK** connector and your **Personal Computer**
  - Start the serial terminal application on the PC and connect via the **Virtual COM Port** of the **ST-LINK**
  - Drag and drop the **EMW3080updateV2.1.11RevC.bin** binary from the subfolder **V2.1.11\\SPI\\** of the previously 
    extracted **MW3080 update tool** package to the **DIS_U585AI** USB mass storage device
  - Flip the **SW2 - BOOT** DIP switch (**not the main BOOT0** DIP switch) to the **0** position instead of **NC 1** position
  - Press the **RST** (black) button to reset the STM32U5 microcontroller that will display a menu in the serial terminal
  - Press the **USER** (blue) button to start the firmware update
  - Wait for programming to finish, serial terminal display should look like below:
    ```
    Boot done successfully
    STM32>Waiting EMW3080 ready for programming (you should wait for more than 20 seconds before getting any message)
    Transferring packet ...
    CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC
    ...
    CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC
    STM32>Programming successful, move back switch to original position, reset the device to get back to prompt
    ```  
  - Return the **SW2 - BOOT** DIP switch to the **NC 1** position
  - Press the **RST** (black) button to reset the STM32U5 microcontroller thus concluding the firmware update

  > Note: When TrustZone is enabled the USB device does not show mass storage device as **DIS_U585AI** but as **NOD_U585AI** which 
    cannot be used for firmware update. To update the EMW3080 firmware the TrustZone needs to be disabled.
