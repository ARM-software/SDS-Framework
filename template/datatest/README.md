# About this application

This is a standalone test application that uses the [SDS Recorder/Player](https://github.com/ARM-software/SDS-Framework)
to record test data and play it back for verification. It contains three solutions that differ only in
the communication technology used for SDS:

- **SDS-Network.csolution.yml** uses the Ethernet interface to connect to the SDSIO server,
- **SDS-USB.csolution.yml** uses the USB interface to connect to SDSIO server,
- **SDS-FileSystem.csolution.yml** saves test files on SD card.

### Projects

Each solution contains two projects:

- **RecordTest.cproject.yml** creates SDS test files that are used to verify bandwidth
- **PlayTest.cproject.yml** plays the SDS test files and checks if the playback works

### Measuring the CPU load

Both projects measure the CPU utilization and output this information. This information gives you an
indication of how much CPU time is available for the user application while the SDS is running.

The idle time is the time during which the CPU is not executing the application code. This means that it executes
the code of the idle thread, incrementing the `idle_cnt` counter. The code for incrementing the idle counter is located
in the `osRtxIdle_Thread`:

```c
__NO_RETURN void osRtxIdleThread(void *argument) {
  (void)argument;

  for (;;) {
    idle_cnt++;
  }
}
```

The counter is incremented for one second, then the idle factor is calculated as the ratio between the idle counter
and the evaluated idle counter for the system without load `no_load_cnt` and output on the debug console:

```c
if (++cnt == 10U) {
  cnt = 0U;

  printf("%d%% idle\n",(idle_cnt - prev_cnt) / no_load_cnt);
  prev_cnt = idle_cnt;
}
```

For a correct measurement, the loop interval must really be periodic, therefore the function `osDelayUntil` is used
to create time intervals in the measurement loop:

```c
timestamp = osKernelGetTickCount();
for (;;) {
   :
  timestamp += 100U;
  osDelayUntil(timestamp);
}
```
