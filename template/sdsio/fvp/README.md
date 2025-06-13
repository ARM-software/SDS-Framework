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

The Arm Virtual Hardware is configured with the [fvp_config.txt](https://github.com/ARM-software/SDS-Framework/blob/main/template/Board/Corstone-300/fvp_config.txt). You can leave the configuration settings at their default values, a change is not necessary.

An important configuration setting is the path to the Python simulation scripts. The path refers to the solution folder:

```txt
mps3_board.v_path=Board/Corstone-300/vsi/python/
```

## Project Configuration

During the simulation, the SDS files are saved on or read from the Host computer. The following steps are required to configure the project for execution on the AVH-FVP simulator:

- create a [New Solution](https://arm-software.github.io/SDS-Framework/main/template.html#create-new-solution) named `SDS` in an empty folder with VS Code IDE.
- select the active target `AVH-SSE-300` and build the application.
- open a [vcpkg-configuration.json](https://learn.arm.com/learning-paths/embedded-and-microcontrollers/vcpkg-tool-installation/config_creation/)
  file and add the following line to the **"requires"** section:

  ```json
  "arm:models/arm/avh-fvp": "~11.29.27"
  ```

- open a file `tasks.json` and add a simulation task `AVH-FVP`:

  ```json
  {
      "label": "AVH-FVP",
      "type": "shell",
      "command": "FVP_Corstone_SSE-300_Ethos-U55",
      "args": [
          "-f",
          "Board/Corstone-300/fvp_config.txt",
          "${command:cmsis-csolution.getBinaryFile}"
      ],
      "problemMatcher": []
  }
  ```

- run the task `AVH-FVP` from the VS Code.

**Note**

- The installation of a stable Python version 3.9 is required.
