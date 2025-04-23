# SDS Interface

The SDSIO components offer flexible recorder and playback interfaces. You may choose between these interface components that are stored in the folder `./sds/source/sdsio`. These interfaces can be accessed as CMSIS software components for integration into the target system:

```yml
  - component: SDS:IO:Socket                     # Socket Interface (Ethernet or WiFi)
  - component: SDS:IO:VCOM&MDK USB               # USB Interface
  - component: SDS:IO:Serial&CMSIS USART         # USART Interface
  - component: SDS:IO:File System&MDK FS         # Memory card
  - component: SDS:IO:File System&Semihosting    # Simulation or Debugger via Semihosting interface
  - component: SDS:IO:Custom                     # Source code template for custom implementation
```

To simplify usage further, the following pre-configured SDS interface layers in *csolution project format* are available. These connect via various interfaces to the SDSIO Server, which provides read/write access to SDS data files.

- [Ethernet Interface](#layer-sds_interfacenetwork) using the MDK-Middleware Network components.
- [USB Interface](#layer-sds_interfaceusb) using the MDK-Middleware USB components.

![SDS Interface](images/SDSIO.png)

## Layer: SDS_Interface/Network

The [`SDS_Interface/Network/SDS_Interface.clayer`](https://github.com/Arm-Examples/SDS-Examples/tree/main/SDS_Interface/Network) is configured for recording and playback via Ethernet interface. It is using the MDK-Middleware Network component.

## Layer: SDS_Interface/USB

The [`SDS_Interface/USB/SDS_Interface.clayer`](https://github.com/Arm-examples/SDS-Examples/tree/main/SDS_Interface/USB) is configured for recording and playback via USB interface. It is using the MDK-Middleware USB component.
