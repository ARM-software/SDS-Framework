# SDS Template Examples

These SDS templates are a test framework for DSP and ML algorithms. It allows recording and playback of  real-world data streams using physical hardware or on simulation models using [(Arm Virtual Hardware - FVP)](https://github.com/ARM-software/AVH) to an user algorithm under test. The real-world data streams are captured in SDS data files. This enables multiple uses cases:

- Validate and Optimize Algorithms using playback. This allows to repeat test cases with the same data streams.
- The captured data streams can be labeled and used as training data set for AI Models in MLOps systems.

The templates are implemented as [CMSIS-Toolbox Reference Applications](https://open-cmsis-pack.github.io/cmsis-toolbox/ReferenceApplications/) that are hardware agnostic and require a board layer with drivers for the specific target hardware.

The templates differ in the SDSIO interface that is used for SDS data file access:

- [filesystem/SDS_FileSystem](./filesystem/SDS_FileSystem.csolution.yml) uses the [MDK-Middleware](https://github.com/ARM-software/MDK-Middleware) FileSystem component for SDS file I/O.
- [network/SDS_Network](./network/SDS_Network.csolution.yml) uses the [MDK-Middleware](https://github.com/ARM-software/MDK-Middleware) Network component to connect to the [SDSIO Server](../utilities).
- [usb/SDS_USB](./usb/SDS_USB.csolution.yml) uses the [MDK-Middleware](https://github.com/ARM-software/MDK-Middleware) USB Device component to connect to the [SDSIO Server](../utilities).

With a custom SDSIO interface alternative file I/O configurations are possible.

In each SDS templates these files require changes to interface with the DSP and ML algorithm that is tested:

- `algorithm/sds_algorithm_config.h` configures the block size of data streams.
- `algorithm/sds_algorithm_user.c` is the interface to the DSP/ML algorithm under test.
- `algorithm/sds_data_in_user.c` is the interface to the physical data source.

For details see [SDS Template Examples](https://arm-software.github.io/SDS-Framework/main/examples.html) in SDS-Framework documentation.
