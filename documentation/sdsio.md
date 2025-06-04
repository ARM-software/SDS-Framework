# SDSIO Interface

The SDSIO components offer flexible recorder and playback interfaces. You may choose between these interface components that are stored in the folder `./sds/source/sdsio`. These interfaces can be accessed as CMSIS software components for integration into the target system:

```yml
  - component: SDS:IO:Socket                     # Socket Interface (Ethernet or WiFi)
  - component: SDS:IO:USB&MDK USB                # USB Interface
  - component: SDS:IO:Serial&CMSIS USART         # USART Interface
  - component: SDS:IO:File System&MDK FS         # Memory card
  - component: SDS:IO:File System&Semihosting    # Simulation or Debugger via Semihosting interface
  - component: SDS:IO:Custom                     # Source code template for custom implementation
```

To simplify usage further, the following pre-configured SDS interface layers in *csolution project format* are available. These connect via various interfaces to the SDSIO Server, which provides read/write access to SDS data files.

- [Ethernet Interface](#layer-sdsio_network) using the MDK-Middleware Network component.
- [USB Bulk Interface](#layer-sdsio_usb) using the MDK-Middleware USB component.
- [Memory Card Interface](#layer-sdsio_fs) using the MDK-Middleware File System component.

## Layer: sdsio_network

The [`layer/network/sdsio_network.clayer.yml`](https://github.com/ARM-software/SDS-Framework/tree/main/layer/sdsio/network) is configured for recording and playback via Ethernet interface. It uses the MDK-Middleware Network component.

## Layer: sdsio_usb

The [`layer/usb/sdsio_usb.clayer.yml`](https://github.com/ARM-software/SDS-Framework/tree/main/layer/sdsio/usb) is configured for recording and playback via USB Device interface. It uses the MDK-Middleware USB Device component.

## Layer: sdsio_fs

The [`layer/filesystem/SDS_Interface.clayer`](https://github.com/ARM-software/SDS-Framework/tree/main/layer/sdsio/filesystem) is configured for recording and playback to/from the Memory Card. It uses the MDK-Middleware File System component.
