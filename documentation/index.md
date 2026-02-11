# SDS-Framework

This documentation explains the usage of the SDS-Framework for developing DSP, ML, or Edge AI algorithms.

## Target Audience

This user's guide assumes basic knowledge about Cortex-M software development. It is written for embedded software developers that work with C/C++ compiler toolchains and utilize microcontroller devices with Cortex-M processors and Ethos-U NPUs.

## Manual Chapters

- [**Overview**](overview.md) explains the features of the SDS-Framework and outlines the possibilities.
- [**SDS Template Application**](template.md) explains the usage of the SDS-Framework for algorithm testing.
- [**SDSIO Interface**](sdsio.md) describes the various I/O interfaces (USB, Socket, File System) for connecting to the target.
- [**Utilities**](utilities.md) explains Python based utilities that operate with SDS data files for converting, viewing, recording, and playback.
- [**Theory of Operation**](theory.md) explains data stream recording and playback.
- [**API Modules**](SDS_API/modules.md) describes the C interface of the SDS functions that may be used in the target system.

## Revision History

Version            | Description
:------------------|:-------------------------
2.1.0              | [Minor updated release of the SDS-Framework](https://github.com/ARM-software/SDS-Framework/releases/tag/v2.1.0)
2.0.0              | [Major updated release of the SDS-Framework](https://github.com/ARM-software/SDS-Framework/releases/tag/v2.0.0)
1.1.0              | [Minor updated version of the SDS-Framework](https://github.com/ARM-software/SDS-Framework/releases/tag/v1.1.0)
1.0.0              | Initial release of the SDS-Framework
