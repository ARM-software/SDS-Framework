# SDS Template Application

The SDS template application is a test framework for DSP and ML algorithms. It allows recording and playback of real-world data streams using physical hardware or on simulation models using (Arm Virtual Hardware - FVP) to an user algorithm under test. The real-world data streams are captured in SDS data files.

The SDSIO-Layer connects the SDS template application to a communication interface for SDS file I/O operations. The following SDSIO interfaces are pre-configured:

- Ethernet Interface using the MDK-Middleware Network component.
- USB Virtual COM Port Interface using the MDK-Middleware USB component.
- Memory Card Interface using the MDK-Middleware File System component.

With a custom SDSIO interface alternative file I/O configurations are possible.

The template contains two projects:

- **DataTest.cproject.yml** check the SDSIO interface with SDS test data files. Use `sds_algorithm_config.h` to configure parameters.
- **AlgorithmTest.cproject.yml** verify an user algorithm with recording and playback of SDS data files.

For more information refer to [SDS Template Application](https://arm-software.github.io/SDS-Framework/main/template.html) in the documentation.

## Measuring CPU load

Both projects measure the CPU utilization and output this information. This information gives you an
indication of how much CPU time is available for the user application while the SDS is running.

The idle time is the time during which the CPU is not executing the application code. During this time it executes
the idle thread, incrementing the `idle_cnt` counter. The code for incrementing the idle counter is located
in the `osRtxIdle_Thread`:

```c
__NO_RETURN void osRtxIdleThread(void *argument) {
  (void)argument;

  for (;;) {
    idle_cnt++;
  }
}
```
