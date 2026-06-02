# SDS Template Application

The [SDS template application](https://arm-software.github.io/SDS-Framework/main/template.html) is a test framework for DSP and ML algorithms.
It allows recording and playback of real-world data streams using physical hardware and supports playback on FVP simulation models to an user algorithm under test.

The template contains two projects:

- **DataTest.cproject.yml** check the SDSIO interface with SDS test data files. Use `algorithm_config.h` to configure parameters.
- **AlgorithmTest.cproject.yml** verify an user algorithm with recording and playback of SDS data files.

The SDSIO-Layer implements the [SDSIO interface](https://arm-software.github.io/SDS-Framework/main/sdsio.html) that connects to the SDSIO-Server, File System, or VSI3 FVP simulation interface.
For more information refer to:

- [Using USB Interface](https://arm-software.github.io/SDS-Framework/main/sdsio.html#layer-sdsio_usb) for connected to SDSIO-Server via USB.
- [Using Network Interface](https://arm-software.github.io/SDS-Framework/main/sdsio.html#layer-sdsio_network) for connected to SDSIO-Server via Ethernet.
- [Using RTT Interface](https://arm-software.github.io/SDS-Framework/main/sdsio.html#layer-sdsio_rtt) for connected to SDSIO-Server via a debug adapter.
- [Using File System](https://arm-software.github.io/SDS-Framework/main/sdsio.html#layer-sdsio_fs) for connected to SDSIO-Server via a debug adapter.
- [Using FVP Simulation Models](https://arm-software.github.io/SDS-Framework/main/sdsio.html#layer-sdsio_fvp) for regression tests with simulation models.

## Measuring CPU load

Both projects measure the CPU utilization and output this information. This information gives you an
indication of how much CPU time is available for the user application while the SDS is running.

The idle time is the time during which the CPU is not executing the application code. During this time it executes
the idle thread, incrementing the `idle_cnt` counter. The code for incrementing the idle counter when the application
is using CMSIS-RTX RTOS is located in the `osRtxIdleThread`:

```c
__NO_RETURN void osRtxIdleThread(void *argument) {
  (void)argument;

  for (;;) {
    idle_cnt++;
  }
}
```
