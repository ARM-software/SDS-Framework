# SDS with SDS I/O Interface via File System (SD Card)

This layer provides SDS with an I/O interface using the SD card as storage media.
It is implemented with the MDK-Middleware File System component.
It is based on the following components:

- [SDS](https://arm-software.github.io/SDS-Framework/main/SDS_API/group__SDS__Interface.html) data streaming,
- [SDS_IO](https://arm-software.github.io/SDS-Framework/main/SDS_API/group__SDS__IO__Interface.html) SDS I/O interface,
- [CMSIS-Compiler](https://arm-software.github.io/CMSIS-Compiler/latest/index.html) retargeting standard C run-time library file operations,
- [MDK-Middleware File System](https://arm-software.github.io/MDK-Middleware/latest/FileSystem/index.html) file system interface,
- [CMSIS-Driver MCI](https://arm-software.github.io/CMSIS_6/latest/Driver/group__mci__interface__gr.html) physical interface.

## SDS Configuration

The following SDS software components are required:

```yml
  - component: SDS:Stream&CMSIS-RTOS2
  - component: SDS:IO:File System&MDK FS
```

## File System Configuration

The following MDK-Middleware File System software components are required:

```yml
  - component: File System&MDK:CORE
  - component: File System&MDK:Drive:Memory Card
```

## Retargeting

For retargeting of standard C run-time library file operations the CMSIS-Compiler component is required:

```yml
  - component: CMSIS-Compiler:File Interface:MDK-MW File System
```

