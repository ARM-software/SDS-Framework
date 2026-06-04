# SDS with SDSIO via USB

This layer provides SDS with SDSIO using the USB communication.
It is implemented with the MDK-Middleware USB component.
It is based on the following components:

- [SDS](https://arm-software.github.io/SDS-Framework/main/SDS_API/group__SDS__Stream__Interface.html) data streaming,
- [SDSIO](https://arm-software.github.io/SDS-Framework/main/SDS_API/group__SDSIO__Interface.html) SDSIO interface,
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

The default `USBD_Config_0.h` contains placeholder Vendor ID (VID) and Product ID (PID) values:

```c
#define USBD0_DEV_DESC_IDVENDOR         0xC251   // replace with your own VID
#define USBD0_DEV_DESC_IDPRODUCT        0x8007   // replace with your own PID
```

The Vendor ID **0xC251 (Keil)** and Product ID **0x8007** can be used for internal testing environments with limited access.
For production, the Vendor ID (VID) and Product ID (PID) should be vendor-specific.
The Vendor ID must be assigned by the [USB Implementers Forum (USB-IF)](https://www.usb.org/) following the procedure for
[Getting a Vendor ID](https://www.usb.org/getting-vendor-id).

## Usage

For more information refer to [Using USB Interface](https://arm-software.github.io/SDS-Framework/main/sdsio.html#layer-sdsio_usb).
