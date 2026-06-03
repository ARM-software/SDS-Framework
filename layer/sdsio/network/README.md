# SDS with SDSIO via Network (Ethernet)

This layer provides SDS with SDSIO using the Ethernet-based IoT Socket communication.
It is implemented with the MDK-Middleware Network component.
It is based on the following components:

- [SDS](https://arm-software.github.io/SDS-Framework/main/SDS_API/group__SDS__Stream__Interface.html) data streaming,
- [SDSIO](https://arm-software.github.io/SDS-Framework/main/SDS_API/group__SDSIO__Interface.html) SDSIO interface,
- [IoT Socket](https://mdk-packs.github.io/IoT_Socket/latest/index.html) communication interface,
- [MDK-Middleware Network](https://arm-software.github.io/MDK-Middleware/latest/Network/index.html) protocol stack,
- [CMSIS-Driver Ethernet](https://arm-software.github.io/CMSIS_6/latest/Driver/group__eth__interface__gr.html) physical interface.

## SDS Configuration

The following SDS software components are required:

```yml
  - component: SDS:Stream&CMSIS-RTOS2
  - component: SDS:IO:Socket
```

The IP address of the SDSIO-Server must be updated in `sdsio_client_socket_config.h` with the
(`Socket server listing`) address reported by the SDSIO-Server at startup:

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

## Usage

For more information refer [Using Network Interface](https://arm-software.github.io/SDS-Framework/main/sdsio.html#layer-sdsio_network).
