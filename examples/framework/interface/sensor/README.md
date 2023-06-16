# Sensor Driver Interface

Sensor Driver Interface for ML and DSP applications.  

It provides a simple interface to the application and relies on pre-configured sensors. 
Exposed is the sensor configuration and functions to enable/disable the sensor and to read out sensor data.  

Sensor can operate in non-DMA or DMA mode with or without interrupt.

## Sensor Driver API

The API is defined in [sensor_drv.h](include/sensor_drv.h). It features the following functions:
- `sensorGetId`: Gets named sensor identifier which is used in other functions specifying a sensor. 
  Pre-configured sensor are exposed through their names.
- `sensorGetConfig`: Get configuration of a specified sensor.  
  Sensor configuration is exported in the `sensorConfig_t` structure:
  - `name`: Sensor name.
  - `dma_mode`: DMA mode: 1=DMA, 0=non-DMA (FIFO).
  - `sample_size`: Sample size in bytes.
  - `u.fifo` structure in non-DMA mode (FIFO):
    - `sample_interval`: Sample interval in microseconds.
    - `fifo_size`: Sample FIFO size in bytes.
    - `data_threshold`: Data event threshold in number of samples.  
      It specifies when the callback with event `SENSOR_EVENT_DATA` is generated. 
      No event is generated when 0 is specified. 
      The event is generated when the number of samples in the FIFO 
      reaches the specified number of samples. 
  - `u.dma` structure in DMA mode:
    - `block_interval`: Block interval in microseconds.
    - `block_size`: Block size in bytes for DMA mode.
    - `block_num`: Number of blocks for DMA mode.
    >Note: Event `SENSOR_EVENT_DATA` is generated on every block captured.
- `sensorRegisterEvents`: Registers an event callback function for the specified sensor with event mask.
- `sensorEnable`: Enables a specified sensor (start capturing data with implicit data flush).
- `sensorDisable`: Disables a specified sensor (stop capturing data).
- `sensorGetStatus` Get status of specified sensor:
  - Active state: 1=active(enabled), 0=inactive(disabled)
  - Overflow flag (auto cleared)
- `sensorReadSamples`: Read samples from specified sensor when in non-DMA mode.
- `sensorGetBlockData`: Get block data from specified sensor when in DMA mode.

The following reference implementation is provided in [sensor_drv.c](source/sensor_drv.c). 
It features:
- support for up to 8 sensors
- common driver implementation with:
  - hardware configuration specified in [sensor_config.h](template/sensor_config.h)
  - hardware interface specified in [sensor_drv_hw.h](include/sensor_drv_hw.h)
    >Note: requires hardware interface implementation (ex: `sensor_drv_hw.c`).

Sensor hardware interface implementation for using VSI (Virtual Streaming Interface) on Arm Virtual Hardware (AVH) is provided in directory [VSI](../VSI).
