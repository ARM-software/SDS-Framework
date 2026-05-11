# SDS with SDS I/O Interface via VSI (Simulator)

This layer provides SDS with an I/O interface using the VSI Simulation interface on the Arm Virtual Hardware FVP model.
It is based on the following components:

- [SDS](https://arm-software.github.io/SDS-Framework/main/SDS_API/group__SDS__Interface.html) data streaming,
- [SDS_IO](https://arm-software.github.io/SDS-Framework/main/SDS_API/group__SDS__IO__Interface.html) SDS I/O interface,
- [VSI interface](https://arm-software.github.io/AVH/main/simulation/html/group__arm__vsi.html) for simulation.

## SDS Configuration

The following SDS software components are required:

```yml
  - component: SDS:Stream&CMSIS-RTOS2
  - component: SDS:IO:VSI
```

## FVP Configuration

The Arm Virtual Hardware is configured with the [fvp_config.txt](https://github.com/ARM-software/SDS-Framework/blob/main/template/Board/Corstone-300/fvp_config.txt). You can leave the configuration settings at their default values, a change is not necessary.

An important configuration setting is the path to the Python simulation scripts. The path refers to the solution folder:

```txt
mps3_board.v_path=Board/Corstone-300/vsi/python/
```

## Project Configuration

During the simulation, the SDS files are saved on or read from the Host computer. The following steps are required to configure the project for execution on the AVH-FVP simulator:

- create a [New Solution](https://arm-software.github.io/SDS-Framework/main/template.html#create-new-solution) named `SDS` in an empty folder with VS Code IDE.
- select the active target `AVH-SSE-300` build and run the application.
