[![License](https://img.shields.io/github/license/arm-software/SDS-Framework?label)](https://github.com/ARM-software/SDS-Framework/blob/main/LICENSE)
[![Pack](https://img.shields.io/github/actions/workflow/status/Arm-Software/SDS-Framework/pack.yml?logo=arm&logoColor=0091bd&label=Build%20pack)](./.github/workflows/pack.yml)
[![GH-Pages](https://img.shields.io/github/actions/workflow/status/Arm-Software/SDS-Framework/gh-pages.yml?logo=arm&logoColor=0091bd&label=Deploy%20content)](./.github/workflows/gh-pages.yml)
[![Build examples](https://img.shields.io/github/actions/workflow/status/Arm-Software/SDS-Framework/build_examples.yml?logo=arm&logoColor=0091bd&label=Build%20examples)](./.github/workflows/build_examples.yml)

# SDS Framework

The **Synchronous Data Stream (SDS) Framework** implements a data stream management, provides methods and helper tools for developing and optimizing embedded applications that integrate DSP and ML algorithms. This framework may be used stand-alone, but also in combination with [**CMSIS-Stream**](https://github.com/ARM-software/CMSIS-Stream) that allows to combine algorithms using a compute graph.


## Examples

The [SDS template application](https://arm-software.github.io/SDS-Framework/main/template.html) demonstarte the recording and playback of real-world data streams using physical hardware or simulation via [Arm Virtual Hardware - FVP](https://github.com/ARM-software/AVH). Simulation enables cost-effective, automated regression testing on desktops or in cloud-based CI/MLOps pipelines. The real-world data streams are captured in [SDS data files](https://arm-software.github.io/SDS-Framework/main/theory.html#sds-data-files).

Refer to [Overview](./overview/README.md) for more details.


## Related

- The [SDS Pack](https://www.keil.arm.com/packs/sds-arm) is available on [keil.arm.com/packs](https://www.keil.arm.com/packs).
- The [SDS Examples](https://github.com/Arm-Examples/sds-examples) is a repository with SDS examples configured for various Evaluation Boards and use the [MDK-Middleware](https://www.keil.arm.com/packs/mdk-middleware-keil/overview/) for the[SDSIO Interface](https://arm-software.github.io/SDS-Framework/main/sdsio.html).
- [ML Developers Guide for Cortex-M Processors and Ethos-U NPU](https://developer.arm.com/documentation/109267).
- [Arm Virtual Hardware - FVP](https://github.com/arm-software/avh) repository with documentation.


## Files and Directories

This is a list of the relevant files and directories.

| Directory                                | Description |
|---                                       |--- |
| [overview](./overview)                   | Top-level overview of SDS Framework. |
| [documentation](./documentation/)        | [User documentation](https://arm-software.github.io/SDS-Framework/main/index.html) of the SDS Framework. |
| [template](./template)                   | [SDS template application](https://arm-software.github.io/SDS-Framework/main/template.html), a test framework for DSP and ML algorithms. |
| [layer/sdsio](./layer/sdsio)             | Configured [SDSIO layers](https://arm-software.github.io/SDS-Framework/main/sdsio.html) for file I/O via Network, USB, or File System. |
| [utilities](./utilities)                 | Python scripts for processing of SDS binary data files. |
| [schema](./schema)                       | Schema for [SDS YAML metadata format](https://arm-software.github.io/SDS-Framework/main/theory.html#yaml-metadata-format) that describes the content of SDS files. |
| [sds](./sds)                             | SDS-Framework source files and implementation of various SDSIO interfaces. |
| [.github/workflows](./.github/workflows) | GitHub Actions for validation and publishing. |
| [.ci](./.ci)                             | Files that relate to CI tests. |


## Continuous Integration (CI)

The underlying build system of [Keil Studio](https://www.keil.arm.com/) uses the [CMSIS-Toolbox](https://open-cmsis-pack.github.io/cmsis-toolbox/) and CMake. [CI](https://en.wikipedia.org/wiki/Continuous_integration) is effectively supported with:
- Tool installation based on a single [`vcpkg-configuration.json`](./vcpkg-configuration.json) file for desktop and CI environments.
- CMSIS solution files (`*.csolution.yml`) that enable seamless builds in CI, for example using GitHub actions.

| CI Workflow                                               | Description                                |
|---                                                        |---                                         |
| [pack](./.github/workflows/pack.yml)                      | Builds the pack on a GitHub hosted runner. |
| [gh-pages](./.github/workflows/gh-pages.yml)              | Deploying this content to GitHub Pages.    |
| [build_examples](./.github/workflows/build_examples.yml)  | Builds the application binaries by using the Arm Compiler for Embedded (AC6) on a GitHub hosted runner. |


## License

The SDS Framework is licensed under [![License](https://img.shields.io/github/license/arm-software/sds-framework?label)](https://github.com/ARM-software/sds-framework/blob/main/LICENSE).


## Documentation

The [documentation](https://arm-software.github.io/SDS-Framework/main/index.html) is generated by using [Doxygen](https://www.doxygen.nl/) and [MKDocs](https://www.mkdocs.org/) with the following additional plugins:

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
