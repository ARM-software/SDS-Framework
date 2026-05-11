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

### USB Vendor ID and Product ID

The default `USBD_Config_0.h` contains placeholder Vendor ID (VID) and Product ID (PID) values that **must** be replaced before deployment:

```c
#define USBD0_DEV_DESC_IDVENDOR         0xC251   // replace with your own VID
#define USBD0_DEV_DESC_IDPRODUCT        0x8007   // replace with your own PID
```

- **VID**: Must be a VID assigned to your organization by [USB-IF](https://www.usb.org/getting-vendor-id). Do not use the Keil VID (`0xC251`) in a product.
- **PID**: Assign a product-specific value within your VID namespace.

## Starting SDSIO Server

The **SDSIO Server** is a Python-based utility for PC, which is included in the
[SDS-Framework](https://github.com/ARM-software/SDS-Framework/tree/main/utilities) pack.

To start the SDSIO Server, run it from the `./utilities` directory with:

```txt
python sdsio-server.py usb
```

Further information about the SDSIO Server application can be found in the
[SDS-Framework documentation](https://github.com/ARM-software/SDS-Framework/tree/main/documentation/utilities.md#sdsio-server).
