# Theory of Operation

<!-- markdownlint-disable MD013 -->
<!-- markdownlint-disable MD036 -->

The core of the SDS-Framework is a circular buffer handling that is the interface between the Recorder/Playback functions and the file I/O communication as shown in the picture below. The user application may read one data streams and write another data stream.

![Theory of operation](images/Theory_of_Operation.png)

ToDo:

- When does sdsRecInit require an event handler?

**Example:** Recording of an accelerometer data stream

The following code snippets show the usage of the **Recorder Interface**.

```c
// *** variable definitions ***
struct {                          // sensor data stream format
  uint16_t x;
  uint16_t y;
  uint16_t z;
} accelerometer [30];             // number of samples in one data stream record

sdsRecId_t *accel_id,             // data stream id
uint8_t accel_buf[1500];          // data stream buffer for circular buffer handling
     :
// *** function calls ***
   sdsRecInit(NULL);              // init SDS Recorder  
     :
   // open data stream for recording
   accel_id = sdsRecOpen("Accel", accel_buf, sizeof(accel_buf), 2*(sizeof(accelerometer));
     :
   // write data in accelerometer buffer with timestamp from RTOS kernel.
   timestamp = osKernelGetTickCount();
   n = sdsRecWrite(accel_id, timestamp, accelerometer, sizeof(accelerometer));
   if (n != sizeof(accelerometer)) {
     ... // unexpected size returned, error handling
   }
     :
  sdsRecClose (accel_id);         // close data stream 
```

## Buffer Size

The size of the data stream buffer depends on several factors such as:

- the communication interface used as the technology may impose certain buffer sizes to maximize the transfer rate.
- the size of the data stream as it is recommended that the buffer is at least twice the size of a single data stream.
- the frequency of the algorithm execution. Fast execution speeds may require a larger buffer.

**Recommended Buffer Size**

The table below contains recommended buffer sizes depending on the communication technology used:

ToDo

Communication  | Buffer Size | Description
:--------------:-------------|:----------------
Network        | 4096        | Default size of Ethernet record is xxxx bytes
USB Device     | xxx         | .
FileSystem     | xxx         | .

## Timestamp

The timestamp is a 32-bit unsigned value and is used for:

- Alignment of different data streams that have the same timestamp value.
- Order of the SDS data files captured during execution.
- Combining multiple SDS file records with the same timestamp value.

The same timestamp connects different SDS file records. It is therefore useful to
use the same timestamp for the recording of one iteration of a DSP or ML algorithm.
In most cases the granularity of an RTOS tick (typically 1ms) is a good choice for a timestamp value.

## SDS File Format

The SDS file format is described [here](https://github.com/ARM-software/SDS-Framework/tree/main/schema).  Each call to 
`sdsRecWrite` creates one record.
