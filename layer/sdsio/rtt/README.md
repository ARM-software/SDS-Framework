# SDS with SDSIO via RTT

This layer provides SDS with SDSIO using the RTT (Real Time Transfer) communication.
It is implemented with the SEGGER RTT component.
It is based on the following components:

- [SDS](https://arm-software.github.io/SDS-Framework/main/SDS_API/group__SDS__Stream__Interface.html) data streaming,
- [SDSIO](https://arm-software.github.io/SDS-Framework/main/SDS_API/group__SDSIO__Interface.html) SDSIO interface,
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

The RTT channel number used for SDSIO is configured in
`./RTE/SDS/sdsio_client_rtt_config.h` with the `SDSIO_RTT_CHANNEL` define (default: `1`).

## Usage

For more information refer [Using RTT Interface](https://arm-software.github.io/SDS-Framework/main/sdsio.html#layer-sdsio_rtt).
