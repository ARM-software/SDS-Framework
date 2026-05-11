# SDS with SDS I/O Interface via USB

This layer provides SDS with an I/O interface using the USB communication.
It is implemented with the MDK-Middleware USB component.
It is based on the following components:

- [SDS](https://arm-software.github.io/SDS-Framework/main/SDS_API/group__SDS__Interface.html) data streaming,
- [SDS_IO](https://arm-software.github.io/SDS-Framework/main/SDS_API/group__SDS__IO__Interface.html) SDS I/O interface,
- [MDK-Middleware USB](https://arm-software.github.io/MDK-Middleware/latest/USB/USB_Device.html) communication stack,
- [CMSIS-Driver USB Device](https://arm-software.github.io/CMSIS_6/latest/Driver/group__usbd__interface__gr.html) physical interface.

## SDS Configuration

The following SDS software components are required:

```yml
  - component: SDS:Stream&CMSIS-RTOS2
  - component: SDS:IO:USB&MDK USB
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
