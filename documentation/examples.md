# Examples

<!-- markdownlint-disable MD013 -->
<!-- markdownlint-disable MD036 -->

SDS examples applications are provided in a separate [GitHub repository](https://github.com/Arm-Examples/sds-examples) and shows the usage of the SDS-Framework.

![SDS Examples](images/Example_Structure.png)

The examples use [*csolution project layers*](https://open-cmsis-pack.github.io/cmsis-toolbox/build-overview/#software-layers) to simplify targeting to different hardware boards.

The **Sensor Layer** implements a data streaming interface to is described in the section [**Overview**](overview.md). It is user code that is application specific.

The **Layer Type: SDS_Recorder** connects the data streams to the SDS files. There are various ready to use implementations available as outlined in the table below. Several layer implementations use the [MDK-Middleware](https://www.keil.arm.com/packs/mdk-middleware-keil/overview/) for communication.

Layer Type: SDS_Recorder        | Description
:-------------------------------|:----------------
`SDS_Rec_Network.clayer.yml`    | Connects via TCP/IP Network Ethernet to SDSIO Server.
`SDS_Rec_USB.clayer.yml`        | Connects via USB Device VCom to SDSIO Server.
`SDS_Rec_FileSystem.clayer.yml` | Connects via FileSystem to a memory card.

The **Layer Type: Board** is a standard board layer that provides a communication interface (`CMSIS_ETH`, `CMSIS_USB_Device`), a sensor interface (`I2C`, `SPI`), and a `CMSIS_VIO` interface for SDS recorder control. Other variants may have Audio or Video interfaces. [Connections](https://open-cmsis-pack.github.io/cmsis-toolbox/ReferenceApplications/#connections) help to identify the compatible software layers.

The [CMSIS-Toolbox section Reference Applications](https://open-cmsis-pack.github.io/cmsis-toolbox/ReferenceApplications) for more information about the project structure.
