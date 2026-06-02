# SDS with SDSIO via VSI (Simulator)

This layer provides SDS with SDSIO using the VSI Simulation interface on the Arm Virtual Hardware FVP model.
It is based on the following components:

- [SDS](https://arm-software.github.io/SDS-Framework/main/SDS_API/group__SDS__Stream__Interface.html) data streaming,
- [SDSIO](https://arm-software.github.io/SDS-Framework/main/SDS_API/group__SDSIO__Interface.html) SDSIO interface,
- [VSI interface](https://arm-software.github.io/AVH/main/simulation/html/group__arm__vsi.html) for simulation.

## SDS Configuration

The following SDS software components are required:

```yml
  - component: SDS:Stream&CMSIS-RTOS2
  - component: SDS:IO:VSI
```

## FVP Configuration

The Arm Virtual Hardware is configured for **Corstone-300** with the [fvp_config.txt](https://github.com/ARM-software/SDS-Framework/blob/main/template/Board/Corstone-300/fvp_config.txt) and for **Corstone-320** with the with the [fvp_config.txt](https://github.com/ARM-software/SDS-Framework/blob/main/template/Board/Corstone-320/fvp_config.txt).
You can leave the configuration settings at their default values, a change is not necessary.

An important configuration setting is the path to the Python simulation scripts. The path refers to the solution folder:

- For **Corstone-300 with Ethos-U55**:

  ```txt
  mps3_board.v_path=./Board/Corstone-300/vsi/python/
  ```

- For **Corstone-320 with Ethos-U85**:

  ```
  mps4_board.v_path=./Board/Corstone-320/vsi/python/
  ```

## Project Configuration

During the simulation, the SDS files are saved on or read from the host computer. The following steps are required to configure the project for execution on the AVH-FVP simulator:

- create a [New Solution](https://arm-software.github.io/SDS-Framework/main/template.html#create-new-solution) named `SDS` in an empty folder with VS Code IDE.
- select the active target `SSE-300-U55` or `SSE-320-U85`, build and run the application.
