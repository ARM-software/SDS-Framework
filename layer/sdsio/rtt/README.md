# SDS with SDS I/O Interface via RTT

This layer provides SDS with an I/O interface using the RTT (Real Time Transfer) communication.
It is implemented with the SEGGER RTT component.
It is based on the following components:

- [SDS](https://arm-software.github.io/SDS-Framework/main/SDS_API/group__SDS__Interface.html) data streaming,
- [SDS_IO](https://arm-software.github.io/SDS-Framework/main/SDS_API/group__SDS__IO__Interface.html) SDS I/O interface,
- [RTT](https://www.segger.com/products/debug-probes/j-link/technology/about-real-time-transfer/) communication channel.

## SDS Configuration

The following SDS software components are required:

```yml
  - component: SDS:Stream&CMSIS-RTOS2
  - component: SDS:IO:RTT
```

## RTT Configuration

The following SEGGER RTT software components are required:

```yml
  - component: SEGGER:RTT
```

## Starting SDSIO Server

The **SDSIO Server** is a Python-based utility for PC, which is included in the
[SDS-Framework](https://github.com/ARM-software/SDS-Framework/tree/main/utilities) pack.

To start the SDSIO Server, run it from the `./utilities` directory with:

```txt
python sdsio-server.py socket --ipaddr 127.0.0.1
```

Further information about the SDSIO Server application can be found in the
[SDS-Framework documentation](https://github.com/ARM-software/SDS-Framework/tree/main/documentation/utilities.md#sdsio-server).
