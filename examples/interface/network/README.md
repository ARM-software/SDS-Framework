# SDS Interface - Network (Ethernet)

This SDS Interface uses the Ethernet-based IoT Socket communication implemented with the MDK-Middleware Network component.
It is based on the following components:

- [SDS Recorder and Player](https://arm-software.github.io/SDS-Framework/main/SDS_API/group__SDS__Recorder__Player.html) data streaming,
- [IoT Socket](https://mdk-packs.github.io/IoT_Socket/latest/index.html) communication interface,
- [MDK-Middleware Network](https://arm-software.github.io/MDK-Middleware/latest/Network/index.html) protocol stack,
- [CMSIS-Driver Ethernet](https://arm-software.github.io/CMSIS_6/latest/Driver/group__eth__interface__gr.html) physical interface.

## SDS Configuration

The following SDS software components are required:

```yml
  - component: SDS:Buffer
  - component: SDS:IO:Socket
  - component: SDS:RecPlay&CMSIS-RTOS2
```

The IP address of the SDSIO server must be updated in `sdsio_config_socket.h` with the
address reported by the SDSIO Server at startup:

```c
#define SDSIO_SOCKET_SERVER_IP    "0.0.0.0"
```

You can leave the other configuration settings at their default values.

## IoT Socket Configuration

The following IoT Utility software components are required:

```yml
  - component: IoT Utility:Socket:MDK Network
```

No further configuration settings are required.

## Network Configuration

The following MDK-Middleware Network software components are required:

```yml
  - component: Network&MDK:CORE
  - component: Network&MDK:Interface:ETH
  - component: Network&MDK:Service:DNS Client
  - component: Network&MDK:Socket:BSD
  - component: Network&MDK:Socket:TCP
  - component: Network&MDK:Socket:UDP
```

The communication does not use the **IPv6** protocol, therefore the variant `IPv4 only` can be selected in `Net_Config.h`:

```c
#define NET_CORE_VARIANT        0
```

You can leave the other configuration settings at their default values.

## Starting SDSIO Server

The **SDSIO Server** is a Python-based utility for PC, which is included in the
[SDS-Framework](https://github.com/ARM-software/SDS-Framework/tree/main/utilities) pack.

To start the SDSIO Server, run it from the `./utilities` directory with:

```txt
python sdsio-server.py socket
```

This activates the Server with the default settings. Further information about the SDSIO Server application
can be found in the [SDS-Framework documentation](https://github.com/ARM-software/SDS-Framework/tree/main/documentation/utilities.md#sdsio-server).

> ***Important Note***: The evaluation board and the PC must be in the same network in order to establish a connection.
