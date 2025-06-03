# SDS Interface - USB

This SDS Interface uses the USB Device (Custom Class) communication implemented with the MDK-Middleware USB component.
It is based on the following components:

- [SDS Recorder and Player](https://arm-software.github.io/SDS-Framework/main/SDS_API/group__SDS__Recorder__Player.html) data streaming,
- [MDK-Middleware USB](https://arm-software.github.io/MDK-Middleware/latest/USB/USB_Device.html) communication stack,
- [CMSIS-Driver USB Device](https://arm-software.github.io/CMSIS_6/latest/Driver/group__usbd__interface__gr.html) physical interface.

## SDS Configuration

The following SDS software components are required:

```yml
  - component: SDS:Buffer
  - component: SDS:IO:USB&MDK USB
  - component: SDS:RecPlay&CMSIS-RTOS2
```

## USB Configuration

The following MDK-Middleware USB software components are required:

```yml
  - component: USB&MDK:CORE
  - component: USB&MDK:Device
  - component: USB&MDK:Device:Custom Class
```

## Starting SDSIO Server

The **SDSIO Server** is a Python-based utility for PC, which is included in the
[SDS-Framework](https://github.com/ARM-software/SDS-Framework/tree/main/utilities) pack.

To start the SDSIO Server, run it from the `./utilities` directory with:

```txt
python sdsio-server.py usb
```

Further information about the SDSIO Server application can be found in the
[SDS-Framework documentation](https://github.com/ARM-software/SDS-Framework/tree/main/documentation/utilities.md#sdsio-server).
