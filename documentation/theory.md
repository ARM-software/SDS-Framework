# Theory of Operation

<!-- markdownlint-disable MD013 -->
<!-- markdownlint-disable MD036 -->

The core of the SDS-Framework is a circular buffer handling that is the interface between the Recorder/Playback functions and the file I/O communication as shown in the picture below. The user application may read one data streams and write another data stream.

![Theory of operation](images/Theory_of_Operation.png)

ToDo:
- describe timestamp, I/O communication, buffer size calculation for sds.c
- When does sdsRecInit require an event handler?

**Example:** Recording of an accelerometer data stream

The following code snippets show the usage of the **Recorder Interface**.

```c
// sensor data stream definition
struct {
  uint16_t x;
  uint16_t y;
  uint16_t z;
} accelerometer [30];

// buffer for circular buffer handling
static uint8_t accel_buf[1500];

// data stream id
sdsRecId_t *accel_id,


// init SDS Recorder 
sdsRecInit(NULL);

// open data stream for recording
accel_id = sdsRecOpen("Accel", accel_buf, sizeof(accel_buf), 2*(sizeof(accelerometer));

// write data in accelerometer buffer with timestamp from RTOS kernel.
timestamp = osKernelGetTickCount();
n = sdsRecWrite(accel_id, timestamp, accelerometer, sizeof(accelerometer));
if (n != sizeof(accelerometer)) {
  ... // unexpected size returned, error handling
}
 
// close data stream 
sdsRecClose (accel_id);
```
