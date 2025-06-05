# SDS Interface - Simulator (FVP)

This SDS Interface uses the VSI Simulation interface on the Arm Virtual Hardware FVP model.
It is based on the following components:

- [SDS Recorder and Player](https://arm-software.github.io/SDS-Framework/main/SDS_API/group__SDS__Recorder__Player.html) data streaming,
- [VSI interface](https://arm-software.github.io/AVH/main/simulation/html/group__arm__vsi.html) for simulation.

## SDS Configuration

The following SDS software components are required:

```yml
  - component: SDS:Buffer
  - component: SDS:IO:VSI
  - component: SDS:RecPlay&CMSIS-RTOS2
```

## FVP Configuration

The Arm Virtual Hardware is configured with the [fvp_config.txt](https://github.com/ARM-software/SDS-Framework/blob/main/template/Board/Corstone-300/fvp_config.txt).

An important configuration setting is the path to the Python simulation scripts. The path refers to the project folder:

```txt
mps3_board.v_path=../Board/Corstone-300/vsi/python/
```

You can leave the configuration settings at their default values, a change is not necessary.
