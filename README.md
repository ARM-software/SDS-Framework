# SDS Framework

The **Synchronous Data Stream (SDS) Framework** implements a data stream management, provides methods and helper tools for developing and optimizing embedded applications that integrate DSP and ML algorithms. This framework may be used stand-alone, but also in combination with [**CMSIS-Stream**](https://github.com/ARM-software/CMSIS-Stream) that allows to combine algorithms using a compute graph.

## Features

- Implements a flexible data stream management for sensor, audio, and video interfaces that process data streams.
- Supports data streams from multiple interfaces (i.e. for sensor fusion) including provisions for time drifts.
- **Record real-world data** with original data sources of the target hardware for analysis and development.
- **Playback real-world data** for algorithm validation using target hardware or [Arm Virtual Hardware - FVP](https://github.com/arm-software/avh).

The captured data streams are stored in SDS data files. A [YAML metadata file](./schema/README.md) can be used to describe the content. The SDS data files have several use cases such as:

- Input to Digital Signal Processing (DSP) development tools such as filter designers
- Input to Machine Learning (ML) model classification, training, and performance optimization 
- Verify execution of DSP algorithm on Cortex-M targets with off-line tools

[Python-based utilities](./utilities/README.md) are provided for recording, playback, visualization, and data conversion

## Repository structure

Directory                         | Description
----------------------------------|-------------------------------
[examples](./examples)            | Example implementations for various evaluation boards
[documentation](./documentation/) | [User documentation](https://arm-software.github.io/SDS-Framework/main/index.html) of the SDS Framework
[schema](./schema)                | Schema for SDS File Format
[sds](./sds)                      | Interfaces of the SDS Framework for Cortex-M devices
[utilities](./utilities)          | Python scripts for processing of SDS binary data files

## Related

- [SDS Examples](https://github.com/Arm-Examples/sds-examples)
- [ML Developers Guide for Cortex-M Processors and Ethos-U NPU](https://developer.arm.com/documentation/109267)
- [Arm Virtual Hardware - FVP](https://github.com/arm-software/avh)

## License

The SDS Framework is licensed under [![License](https://img.shields.io/github/license/arm-software/sds-framework?label)](https://github.com/ARM-software/sds-framework/blob/main/LICENSE).

## Documentation

The [documentation](https://arm-software.github.io/SDS-Framework/main/index.html) is generated using [Doxygen](https://www.doxygen.nl/) and [MKDocs](https://www.mkdocs.org/) with the following additional plugins:

- [mermaid2](https://mkdocs-mermaid2.readthedocs.io/en/latest/) for sequence diagrams.
- [mkdoxy](https://pypi.org/project/mkdoxy) for API documentation.

Use `mkdocs serve` to generate the documentation on a local computer.

## Contributions and Pull Requests

Contributions are accepted under [![License](https://img.shields.io/github/license/arm-software/CMSIS_6?label)](https://github.com/ARM-software/CMSIS_6/blob/main/LICENSE). Only submit contributions where you have authored all of the code.

### Issues and Labels

Please feel free to raise an [issue on GitHub](https://github.com/ARM-software/sds-framework/issues)
to report misbehavior (i.e. bugs) or start discussions about enhancements. This
is your best way to interact directly with the maintenance team and the community.
We encourage you to append implementation suggestions as this helps to decrease the
workload of the very limited maintenance team.

We will be monitoring and responding to issues as best we can.
Please attempt to avoid filing duplicates of open or closed items when possible.
In the spirit of openness we will be tagging issues with the following:

- **bug** – We consider this issue to be a bug that will be investigated.
- **wontfix** - We appreciate this issue but decided not to change the current behavior.
- **enhancement** – Denotes something that will be implemented soon.
- **future** - Denotes something not yet schedule for implementation.
- **out-of-scope** - We consider this issue loosely related to CMSIS. It might by implemented outside of CMSIS. Let us know about your work.
- **question** – We have further questions to this issue. Please review and provide feedback.
- **documentation** - This issue is a documentation flaw that will be improved in future.
- **review** - This issue is under review. Please be patient.
- **DONE** - We consider this issue as resolved - please review and close it. In case of no further activity this issues will be closed after a week.
- **duplicate** - This issue is already addressed elsewhere, see comment with provided references.
- **Important Information** - We provide essential information regarding planned or resolved major enhancements.
