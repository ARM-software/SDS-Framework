# SDS-Framework

This documentation explains the usage of the SDS-Framework for developing DSP, ML, or Edge AI algorithms.

## Target Audience

This user's guide assumes basic knowledge about Cortex-M software development. It is written for embedded software developers that work with C/C++ compiler toolchains and utilize microcontroller devices with Cortex-M processors and Ethos-U NPUs.

## Manual Chapters

- [**Overview**](overview.md) explains the of features of the SDS-Framework and outlines the possibilities.
- [**Theory of Operation**](theory.md) explains data stream recording and playback.
- [**SDS Interface**](sds_interface.md) describes the various interfaces (USB, Socket, File System) for connecting to the target.
- [**SDS Template Application**](template.md) explains the usage of the SDS-Framework for algorithm testing.
- [**Utilities**](utilities.md) explains Python based utilities that operate with SDS data files for converting, viewing, recording, and playback.
- [**API Modules**](SDS_API/modules.md) describes the C interface of the SDS functions that may be used in the target system.

## Revision History

Version            | Description
:------------------|:-------------------------
2.0.0              | Initial release for SDS-Framework [2.0.0](https://github.com/ARM-software/SDS-Framework/releases/tag/v2.0.0)
