# SDS Interface - File System (SD Card)

This SDS Interface uses the SD Card as media and is implemented with the MDK-Middleware File System component.
It is based on the following components:

- [SDS Recorder and Player](https://arm-software.github.io/SDS-Framework/main/SDS_API/group__SDS__Recorder__Player.html) data streaming,
- [CMSIS-Compiler](https://arm-software.github.io/CMSIS-Compiler/latest/index.html) retargeting standard C run-time library file operations,
- [MDK-Middleware File System](https://arm-software.github.io/MDK-Middleware/latest/FileSystem/index.html) file system interface,
- [CMSIS-Driver MCI](https://arm-software.github.io/CMSIS_6/latest/Driver/group__mci__interface__gr.html) physical interface.

## SDS Configuration

The following SDS software components are required:

```yml
  - component: SDS:Buffer
  - component: SDS:IO:File System&MDK FS
  - component: SDS:RecPlay&CMSIS-RTOS2
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

