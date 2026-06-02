# Theory of Operation

<!-- markdownlint-disable MD013 -->
<!-- markdownlint-disable MD036 -->
<!-- markdownlint-disable MD060 -->

The SDS Framework enables to record and playback one or more data streams to an application that is under development as shown in the diagram below. With the SDSIO Interface the data streams are connected to SDS data files. The file storage can be either embedded within the system and accessed by a file system or external on a host computer and accessed by a communication interface such as Ethernet or USB.

The DSP or ML algorithms that are tested operate on blocks and are executed periodically. This documentation uses these terms:

- **Data Block**: is a set of input or output data which is processed in one step by a DSP or ML compute note.
- **Block size**: is the number of bytes of a data block.
- **Interval**: is the periodic time interval at which the compute node executes.

![SDSIO Interface for Player and Recorder](images/SDS-InOut.png)


Using the SDS Stream Interface functions (`sds.c/h`), the data stream under development may read and write as shown above. The Stream Interface functions store data streams in a circular buffer (`sds_buffer.c/h`). This circular buffer is the I/O queue for the SDSIO-Client (`sdsio_x.c / sdsio.h`). 

![Implementation Files of SDS](images/Theory_of_Operation.png)

## Usage

The following diagram shows the usage of the SDS Stream Interface functions (executed in `sdsThread`).  The `sdsControlThread` controls the overall execution. `AlgorithmThread` is the thread that executes Signal Conditioning (SC) and ML Model.

When `AlgorithmThread` starts, it first calls the `InitInputData` function, which initializes the input interfaces
(for example, camera, microphone, accelerometer, etc.). It then calls `InitAlgorithm`, which is responsible for initializing the ML algorithm.

After initialization, `AlgorithmThread` enters a loop in which it repeatedly calls `GetInputData`. This function provides a block of input data
used for a single inference. The inference itself is executed by the `ExecuteAlgorithm` function.

Two additional functions are also provided:

- `DiscardInputData`: used during playback to discard incoming live input data, since the actual input is read from the SDS file.
- `ResetAlgorithm`: called before each playback run to reset internal states, memory buffers, and results from previous inferences,
  ensuring a clean start for the new playback session.

```mermaid
sequenceDiagram
    participant sdsControlThread
    participant sdsThread
    participant AlgorithmThread
    Note over sdsControlThread: sdsInit
    activate sdsThread
    Note over sdsControlThread: Open data streams
    Note over sdsThread: Read content for<br/>'play' data streams
    activate AlgorithmThread
    loop periodic
        Note over AlgorithmThread: GetInputData<br/>(physical input or sdsRead)
        Note over AlgorithmThread: Execute algorithm
        Note over AlgorithmThread: sdsWrite data streams
        Note over sdsThread: Read/write data streams.
    end
    sdsControlThread-->>AlgorithmThread: Stop Algorithm
    AlgorithmThread-->>sdsControlThread: Stopped
    Note over sdsControlThread: Close data streams
    Note over sdsThread: Flush and close<br/> data streams
```

## SDS Data Files

Each data stream is stored in a separate SDS data file. In the diagram below `SCinput.0.sds` is the input to Signal Conditioning, `SCoutput.0.sds` is the output of Signal Conditioning, and `MLoutput.0.sds` is the output of the ML Model. Each execution of the algorithm is represented in a data block with a `timeslot`. The `timeslot` allows to correlate the blocks of different streams. In the above example, all blocks of one algorithm execution have the same timeslot value.

![SDS Files](images/SDS-Files.png)

- Each call to the function `sdsWrite` writes one complete data block.
- Each call to the function `sdsRead` reads one complete data block.

### Filenames

The `sdsOpen` function takes `<name>` for the stream and the opening mode as input parameters.
Opening a stream in `sdsModeRead` mode is used for playback and opening stream in `sdsModeWrite` is used for recording.

The actual files used when opening a stream depends on the presence of the [`*.sdsio.yml` control file](utilities.md#sdsio-control-file-sdsioyml) and its `play` node.

**Recording:**

`<label>` is a sequential number starting at 0 and incremented by 1. The first value for which no corresponding file exists is used to create a new file.
After recording completes, the process continues from the last `<label>` value.

**Playback:**

When a [`*.sdsio.yml` control file](utilities.md#sdsio-control-file) is used and contains [`play:`](utilities.md#play) node, the filename follows the pattern `<stream-name>.<label>.sds`, where `<label>` is specified in the corresponding
[`step:`](utilities.md#play).

When no `*.sdsio.yml` control file is used, the `<label>` is a sequential number starting at 0. If the corresponding file does not exist, the open operation fails.
After playback completes, the process repeats with the `<label>` incremented by one.

!!! Note
    - Files recorded during playback include an additional `.p` before the `.sds` extension to
      distinguish them from originally recorded files (e.g., `ML_Out.0.p.sds`).
    - If a recording filename already exists, any existing `.bak` file with the same name is deleted,
      the current file is renamed by appending `.bak`, and the new recording uses the original filename.

### Timeslot

The timeslot is a 32-bit unsigned value and is used for:

- Alignment of different data streams that have the same timeslot value.
- Order of the SDS data files captured during execution.
- Combining multiple SDS file records with the same timeslot value.

The same timeslot connects different SDS file records. It is therefore useful to
use the same timeslot for the recording of one iteration of a DSP or ML algorithm.
In most cases the granularity of an RTOS tick (typically 1ms) is a good choice for a timeslot resolution.

### File Format

The **SDS Framework** uses a binary data file format to store the individual data streams. It supports the recording and playback of multiple data streams that may have jitters.  Therefore each stream contains timeslot information that allows to correlate the data streams as it is for example required in a sensor fusion application.

The binary data format (stored in `*.sds` data files) has a record structure with a variable size. Each record has the following format:

1. **timeslot**: record timeslot in tick-frequency (32-bit unsigned integer, little endian)
2. **data size**: number of data bytes in the record (32-bit unsigned integer, little endian)
3. **binary data**: SDS stream (little endian, no padding) as described with the `*.sds.yml` file.

### YAML Metadata Format

The content of each data stream may be described in a [YAML](https://en.wikipedia.org/wiki/YAML) metadata file that is created by the user. The following section defines the YAML format of this metadata file. The file `schema/sds.schema.json` is a schema description of the SDS Format Description.

`sds:`                               | Start of the SDS format description
:------------------------------------|---------------------------------------------------
&nbsp;&nbsp;&nbsp; `name:`           | Name of the Synchronous Data Stream (required)
&nbsp;&nbsp;&nbsp; `description:`    | Additional descriptive text (optional)
&nbsp;&nbsp;&nbsp; `frequency:`      | Capture frequency of the SDS (required)
&nbsp;&nbsp;&nbsp; `tick-frequency:` | Tick frequency of the timeslot value (optional); default: 1000
&nbsp;&nbsp;&nbsp; `content:`        | List of values captured (required, see below)

`content:`                           | List of values captured (in the order of the data file)
:------------------------------------|---------------------------------------------------
`- value:`                           | Name of the value (required)
&nbsp;&nbsp;&nbsp; `type:`           | Data type of the value (required)
&nbsp;&nbsp;&nbsp; `offset:`         | Offset of the value (optional); default: 0
&nbsp;&nbsp;&nbsp; `scale:`          | Scale factor of the value (optional); default: 1.0
&nbsp;&nbsp;&nbsp; `unit:`           | Physical unit of the value (optional)
&nbsp;&nbsp;&nbsp; `image:`          | Image stream metadata (optional, see below)

`image:`                             | [Image metadata](#image-metadata-format) for one `content` entry
:------------------------------------|---------------------------------------------------
&nbsp;&nbsp;&nbsp; `pixel_format:`   | Pixel format identifier (required)
&nbsp;&nbsp;&nbsp; `width:`          | Number of pixels per row (required, integer >= 1)
&nbsp;&nbsp;&nbsp; `height:`         | Number of rows (required, integer >= 1)
&nbsp;&nbsp;&nbsp; `stride_bytes:`   | Bytes per row for single-plane formats (optional)
&nbsp;&nbsp;&nbsp; `planes:`         | Per-plane stride for multi-plane formats (optional, 2..3 entries)

`planes:`                            | Per-plane metadata entry
:------------------------------------|---------------------------------------------------
`- stride_bytes:`                    | Bytes per row for this plane (required, integer >= 1)

For `image`, exactly one of `stride_bytes` or `planes` must be provided.

**Example**

This example defines a data stream with the name "sensorX" that contains the values of a gyroscope, temperature sensor, and additional raw data (that are not further described).

![image](./images/SDS-Metainfo.png)

The binary data that are coming form these sensors are stored in data files with the following file format: `<sensor-name>.<index>.sds`. In this example the files names could be:

```yml
   sensorX.0.sds   # capture 0
   sensorX.1.sds   # capture 1
```

The following `sensorX.sds.yml` provides the format description of the SDS `sensorX` binary data files and may be used by data conversion utilities and data viewers.

```yml
sds:                   # describes a synchronous data stream
  name: sensorX        # user defined name
  description: Gyroscope stream with 1KHz, plus additional user data
  frequency: 1000
  content:
  - value: x           # Value name is 'x'
    type:  uint16_t    # stored using a 16-bit unsigned int
    scale: 0.2         # value is scaled by 0.2
    unit: dps          # base unit of the value
  - value: y
    type: uint16_t
    scale: 0.2
    unit: dps
  - value: z
    type: uint16_t
    unit: dps          # scale 1.0 is default
  - value: temp
    type: float
    unit: degree Celsius
  - value: raw
    type: uint16_t     # raw data, no scale or unit given
  - value: flag
    type: uint32_t:1   # a single bit stored in a 32-bit int
```

#### Image Metadata Format

The `pixel_format` values listed in `sds.schema.json` map to the following template files located in the folder `schema\image_format` and Linux V4L2 references.

`pixel_format:` | Template file | V4L2 reference page
:---------------------|:--------------|:------------------
`RAW8` | `RAW8.sds.yml` | [Luma-Only formats](https://www.kernel.org/doc/html/latest/userspace-api/media/v4l/pixfmt-yuv-luma.html) *(GREY family)*
`RAW10` | `RAW10.sds.yml` | [10-bit Bayer (expanded to 16-bit)](https://www.kernel.org/doc/html/latest/userspace-api/media/v4l/pixfmt-srggb10.html)
`RGB565` | `RGB565.sds.yml` | [RGB formats](https://www.kernel.org/doc/html/latest/userspace-api/media/v4l/pixfmt-rgb.html)
`RGB888` | `RGB888.sds.yml` | [RGB formats](https://www.kernel.org/doc/html/latest/userspace-api/media/v4l/pixfmt-rgb.html) *(RGB24)*
`NV12` | `NV12.sds.yml` | [Planar YUV formats](https://www.kernel.org/doc/html/latest/userspace-api/media/v4l/pixfmt-yuv-planar.html)
`NV21` | `NV21.sds.yml` | [Planar YUV formats](https://www.kernel.org/doc/html/latest/userspace-api/media/v4l/pixfmt-yuv-planar.html)
`I420` | `I420.sds.yml` | [Planar YUV formats](https://www.kernel.org/doc/html/latest/userspace-api/media/v4l/pixfmt-yuv-planar.html) *(YUV420 / YU12 family)*
`NV16` | `NV16.sds.yml` | [Planar YUV formats](https://www.kernel.org/doc/html/latest/userspace-api/media/v4l/pixfmt-yuv-planar.html)
`NV61` | `NV61.sds.yml` | [Planar YUV formats](https://www.kernel.org/doc/html/latest/userspace-api/media/v4l/pixfmt-yuv-planar.html)
`YUYV` | `YUYV.sds.yml` | [Packed YUV formats](https://www.kernel.org/doc/html/latest/userspace-api/media/v4l/pixfmt-packed-yuv.html)
`UYVY` | `UYVY.sds.yml` | [Packed YUV formats](https://www.kernel.org/doc/html/latest/userspace-api/media/v4l/pixfmt-packed-yuv.html)
`YUV422P` | `YUV422P.sds.yml` | [Planar YUV formats](https://www.kernel.org/doc/html/latest/userspace-api/media/v4l/pixfmt-yuv-planar.html)
`YUV444` | `YUV444.sds.yml` | [Packed YUV formats](https://www.kernel.org/doc/html/latest/userspace-api/media/v4l/pixfmt-packed-yuv.html) *(packed 4:4:4)*
`YUV444P` | `YUV444P.sds.yml` | [Planar YUV formats](https://www.kernel.org/doc/html/latest/userspace-api/media/v4l/pixfmt-yuv-planar.html) *(YUV444M family)*

!!! Note
    - When RAW10 is packed (4 pixels in 5 bytes), use [10-bit packed Bayer formats](https://www.kernel.org/doc/html/latest/userspace-api/media/v4l/pixfmt-srggb10p.html).

## Code Example

The following code snippets show the usage of the **SDS** for recording of sensor data. In this case an accelerometer data stream is recorded.

```c
#include "sds.h"

// variable definitions
struct {                        // sensor data stream format
  uint16_t x;
  uint16_t y;
  uint16_t z;
} accelerometer [30];           // number of samples in one data stream record

sdsId_t *accel_id;              // data stream id
uint8_t  accel_buf[(sizeof(accelerometer)*2)+2048];     // data stream buffer for circular buffer handling
uint32_t timeslot;
int32_t  n;                     // number of bytes written to data stream
     :
// *** function calls ***
   sdsInit(NULL);               // init SDS
     :
   // open data stream for writing (recording)
   accel_id = sdsOpen("Accel", sdsModeWrite, accel_buf, sizeof(accel_buf));
     :
   // write data in accelerometer buffer with timeslot from RTOS kernel.
   timeslot = osKernelGetTickCount();
   n = sdsWrite(accel_id, timeslot, accelerometer, sizeof(accelerometer));
   if (n != sizeof(accelerometer)) {
     ... // unexpected size returned, error handling
   }
     :
  sdsClose(accel_id);           // close data stream
```

## Buffer Size

The size of the data stream buffer depends on several factors such as:

- the communication interface technology that may impose specific buffer size requirements to maximize data transfer rates.
- the frequency of the algorithm execution. Fast execution speeds may require a larger buffer.

As a guideline, the buffer size should be at least **(2 × block size) + 2 KB**.  
The minimum recommended buffer size is **0x1000 (4 KB)**.

## SDSIO-Server Protocols

SDSIO-Server communicates with the firmware using the [SDSIO-Server Firmware Protocol](theory.md#sdsio-server-firmware-protocol) over supported physical interfaces,
including Ethernet, Wi-Fi, USB, debugger, and serial (USART) connections.
It communicates with the Monitor program through the [Monitor Interface](theory.md#sdsio-server-monitor-interface) over a network connection,
typically via the loopback (localhost) interface.

![SDSIO-Server Protocols](images/SDSIO-Server-Protocols.png)

## SDSIO-Server Firmware Protocol

The [SDSIO-Server](utilities.md#sdsio-server) uses a simple protocol for data exchange between a host computer and the embedded target that integrates an [SDSIO Interface](sdsio.md). The protocol assumes that the correct communication to the SDSIO-Server is already ensured by the underlying technology (TCP/IP or USB) and therefore no extra check is implemented.

The following conventions describe the **command semantic** used in the following documentation:

Symbol     | Description
:----------|:----------------------
\>         | Prefix indicating the direction: Command from target firmware to SDSIO-Server on the host computer.
<          | Prefix indicating the direction: Response from SDSIO-Server to target firmware.
WORD       | 32-bit value (low byte first).
****       | The field above has exactly one occurrence.
++++       | The field above has a variable length.

**Commands:**

Commands are sent from the embedded target to the host computer running the SDSIO-Server.

ID  | Name            | Description
:--:|:----------------|:------------------------
1   | SDSIO_CMD_OPEN  | Open an SDS data file
2   | SDSIO_CMD_CLOSE | Close an SDS data file
3   | SDSIO_CMD_WRITE | Write to an SDS data file
4   | SDSIO_CMD_READ  | Read from an SDS data file
5   | SDSIO_CMD_PING  | Ping SDSIO-Server
6   | SDSIO_CMD_FLAGS | SDS control flags update request from host
7   | SDSIO_CMD_INFO  | Send control information to host

Each **Command** starts with a **Header (4 Words = 16 bytes)** followed by **optional data** of variable length.
Depending on the Command, the SDSIO-Server replies with a **Response** that includes a **Header** with the same ID
as the Command and may contain **additional data**.

!!! Note
    - The SDSIO_CMD_FLAGS Response is not a reply to the SDSIO_CMD_FLAGS Command; rather, it is an asynchronous Response sent by the host.

**SDSIO_CMD_OPEN**

The Command with ID = **1** (SDSIO_CMD_OPEN) opens an SDS data file on the host computer.
`Mode` defines `read` (value = 0) or `write` (value = 1) operation. `Len of Stream Name` is the size of the string in bytes.

SDS data filenames use the following file format: `<name>.<label>.sds`, where `<name>` is the stream name used as base filename
of the SDS data file and `<label>` is a user label or index depending on usage (for details see section [Filenames](#filenames)).

```txt
| WORD |  WORD  | WORD | WORD **************|+++++++++++++|
>  1   |   0    | Mode | Len of Stream Name | Stream Name |
|******|********|******|********************|+++++++++++++|
```

The Response with ID = **1** (SDSIO_CMD_OPEN) provides a `Handle` that is used to identify the file in subsequent Commands.

```txt
| WORD |  WORD  | WORD | WORD *******|
<  1   | Handle | Mode | 0           |
|******|********|******|*************|
```

**SDSIO_CMD_CLOSE**

The Command with ID = **2** (SDSIO_CMD_CLOSE) closes an SDS data file on the host computer.
The `Handle` is the identifier obtained with **SDSIO_CMD_OPEN**.
There is no Response from the SDSIO-Server to this Command.

```txt
| WORD |  WORD  | WORD | WORD |
>  2   | Handle |  0   |  0   |
|******|********|******|******|
```

**SDSIO_CMD_WRITE**

The Command with ID = **3** (SDSIO_CMD_WRITE) writes data to an SDS data file on the host computer.
The `Handle` is the identifier obtained with **SDSIO_CMD_OPEN**.
`Size` specifies the size of `Data` in bytes.
There is no Response from the SDSIO-Server to this Command.

```txt
| WORD |  WORD  | WORD | WORD |++++++|
>  3   | Handle |  0   | Size | Data |
|******|********|******|******|++++++|
```

**SDSIO_CMD_READ**

The Command with ID = **4** (SDSIO_CMD_READ) reads data from an SDS data file on the host computer.
The `Handle` is the identifier obtained with **SDSIO_CMD_OPEN**.
`Size` specifies the number of bytes to read.

```txt
| WORD |  WORD  | WORD | WORD |
>  4   | Handle | Size |  0   |
|******|********|******|******|
```

The Response with ID = **4** (SDSIO_CMD_READ) provides the data read from an SDS data file on the host computer.
`Size` specifies the size of `Data` in bytes that was read and `Status` with nonzero = end of stream, else 0.

```txt
| WORD |  WORD  |  WORD  | WORD |++++++|
<  4   | Handle | Status | Size | Data |
|******|********|********|******|++++++|
```

**SDSIO_CMD_PING**

The Command with ID = **5** (SDSIO_CMD_PING) verifies if the SDSIO-Server is active and reachable on the host.

```txt
| WORD | WORD | WORD | WORD |
>  5   |  0   |  0   |  0   |
|******|******|******|******|
```

The Response with ID = **5** (SDSIO_CMD_PING) returns the `Status` with nonzero = server active, else 0.

```txt
| WORD | WORD |  WORD  | WORD |
<  5   |  0   | Status |  0   |
|******|******|********|******|
```

**SDSIO_CMD_FLAGS**

The asynchronous Response with ID = **6** (SDSIO_CMD_FLAGS) contains the SDS control flags update information from the host.
This Response can arrive at any time when the host wants to update the SDS control flags.
It can also precede a Response to any other command (e.g., SDSIO_CMD_OPEN or SDSIO_CMD_READ), but it cannot be sent by the host
while a Response to another Command is in progress.
The `Set Mask` specifies the bits to set in the `sdsFlags` and the `Clear Mask` specifies the bits to clear in the `sdsFlags`.

```txt
| WORD | WORD     | WORD       | WORD |
<  6   | Set Mask | Clear Mask |  0   |
|******|**********|************|******|
```

**SDSIO_CMD_INFO**

The Command with ID = **7** (SDSIO_CMD_INFO) sends control information (sdsFlags, sdsIdleRate and error information) to the host. There is no Response from the SDSIO-Server to this Command.

- `sdsFlags` is the current value of that global variable.
- `sdsIdleRate` is the current value of that global variable, value 0xFFFFFFFF indicates that idle rate information is not valid.
- `Error Len` specifies the size of the `Error Data`, value 0 indicates that no error occurred.

```txt
| WORD | WORD     | WORD        | WORD      |++++++++++++|
>  7   | sdsFlags | sdsIdleRate | Error Len | Error Data |
|******|**********|*************|***********|++++++++++++|
```

`Error Data` contains the information from `sdsError` global structure and has the following format.

```txt
| WORD   | WORD |+++++++++++++++++++|
| Status | Line | Filename (string) |
|********|******|+++++++++++++++++++|
```

## SDSIO-Server Monitor Interface

The [SDSIO-Server](utilities.md#sdsio-server) provides an additional TCP socket that maybe used by a Monitor program to observe
SDS file activity and control `sdsFlags` in the firmware.
The monitor interface is enabled with command line option `--mon-port <port>`.

The following conventions describe the **command and message semantic** used in the following documentation:

Symbol     | Description
:----------|:----------------------
\>         | Prefix indicating the direction: Command sent from the Monitor program to the SDSIO-Server.
<          | Prefix indicating the direction: Response or asynchronous message sent from the SDSIO-Server to the Monitor program.
WORD       | 32-bit value (low byte first).
****       | The field above has exactly one occurrence.
++++       | The field above has a variable length.

**Commands and Messages:**

Commands are sent to the SDSIO-Server which replies with a response (depending on command).
SDSIO-Server can also send asynchronous messages at any time when not processing a command.

ID  | Name               | Description
:--:|:-------------------|:------------------------
1   | SDSIO_MON_OPEN     | Information about the SDS file open operation (message)
2   | SDSIO_MON_CLOSE    | Information about the SDS file close operation (message)
6   | SDSIO_MON_FLAGS    | Monitor program request to the SDSIO-Server to update SDS control flags in the firmware
7   | SDSIO_MON_INFO     | Information update received from the firmware and forwarded to the Monitor program (message)
8   | SDSIO_MON_SHUTDOWN | Monitor program request to the SDSIO-Server to complete current tasks and shut down gracefully

Each **Command or Message** starts with a **Header (4 Words = 16 bytes)** followed by **optional data** of variable length.
Depending on the Command, the SDSIO-Server replies with a **Response** that includes a **Header** with the same ID
as the Command and may contain **additional data**.

**SDSIO_MON_OPEN**

The Message with ID = **1** (SDSIO_MON_OPEN) is sent whenever an SDS data file is opened by the SDSIO-Server.
`Mode` defines `read` (value = 0) or `write` (value = 1) operation. `Filename Len` is the size of the string in bytes and `Filename` is the name of the file (including path).

```txt
| WORD | WORD | WORD | WORD | WORD ********|++++++++++|
<  1   |  0   | Mode |  0   | Filename Len | Filename |
|******|******|******|******|**************|++++++++++|
```

**SDSIO_MON_CLOSE**

The Message with ID = **2** (SDSIO_MON_CLOSE) is sent whenever an SDS data file is closed by the SDSIO-Server.
`Filename Len` is the size of the string in bytes and `Filename` is the name of the file (including path).

```txt
| WORD | WORD | WORD | WORD | WORD ********|++++++++++|
<  2   |  0   |  0   |  0   | Filename Len | Filename |
|******|******|******|******|**************|++++++++++|
```

**SDSIO_MON_FLAGS**

The Command with ID = **6** (SDSIO_MON_FLAGS) is used by the Monitor program to request an update of the SDS control flags in the firmware.
The `Set Mask` specifies the bits to set in the `sdsFlags` and the `Clear Mask` specifies the bits to clear in the `sdsFlags`.
This command does not generate a response from the SDSIO-Server.

```txt
| WORD | WORD     | WORD       | WORD |
>  6   | Set Mask | Clear Mask |  0   |
|******|**********|************|******|
```

**SDSIO_MON_INFO**

The Message with ID = **7** (SDSIO_MON_INFO) contains status information received by the SDSIO-Server from the firmware,
including sdsFlags, sdsIdleRate, and error information.
The message is sent to the Monitor program whenever the SDSIO-Server receives updated information from the firmware,
as well as upon the initial connection to the Monitor program.

- `sdsFlags` is the current value of that global variable in the firmware.
- `sdsIdleRate` is the current value of that global variable in the firmware, value 0xFFFFFFFF indicates that idle rate information is not valid.
- `Error Len` specifies the size of the `Error Data`, value 0 indicates that no error occurred in the firmware.

```txt
| WORD | WORD     | WORD        | WORD      |++++++++++++|
<  7   | sdsFlags | sdsIdleRate | Error Len | Error Data |
|******|**********|*************|***********|++++++++++++|
```

`Error Data` contains the information from `sdsError` global structure and has the following format.

```txt
| WORD   | WORD |+++++++++++++++++++|
| Status | Line | Filename (string) |
|********|******|+++++++++++++++++++|
```

**SDSIO_MON_SHUTDOWN**

The Command with ID = **8** (SDSIO_MON_SHUTDOWN) is used by the Monitor program to request the SDSIO-Server to complete any ongoing operations,
signal to the firmware that it will stop operating, and shut down gracefully.

```txt
| WORD | WORD | WORD | WORD |
>  8   |  0   |  0   |  0   |
|******|******|******|******|
```

!!! Note
    Monitor program or user should ensure that no streams are open before the shutdown is initiated.

## SDSIO Message Sequence

This section describes the states and the message sequence of the SDS framework when using the [SDSIO-Server](utilities.md#sdsio-server).
It contains the following threads that execute on the target.

- **sdsControlThread**: Control thread that organizes the overall execution.
- **AlgorithmThread**: Executes the algorithm under test.
- **sdsThread**: SDS worker thread.
- **SDSIO-Server**: SDSIO-Server running on the host computer.

!!! Note
    - The command `SDSIO_CMD_FLAGS` is sent asynchronous by the SDSIO-Server.

The `sdsControlThread` handles a state machine with the following states:

States                   | Description
:------------------------|:------------
SDS_STATE_INACTIVE       | Streaming is not active; waiting for flag info from SDSIO-Server
SDS_STATE_CONNECTED      | Device (client) is connected to SDSIO-Server (host), but streaming is not active
SDS_STATE_START          | Request to start streaming, open streams and get ready for read/write operations
SDS_STATE_ACTIVE         | Streaming is active
SDS_STATE_STOP_REQ       | Request to stop streaming and close open streams
SDS_STATE_STOP_DONE      | Streaming has stopped
SDS_STATE_END            | Request to end streaming (e.g., no more playback data is available)
SDS_STATE_RESET          | Request to reset the device
SDS_STATE_END            | Request to terminate the active session

The following flowcharts show the state transition in context with the messages that are exchanged with the `SDSIO-Server`.

**Connect flowchart**

```mermaid
sequenceDiagram
    participant sdsControlThread
    activate sdsControlThread

    participant Server as SDSIO-Server<br/>(host)

    Note over sdsControlThread: sdsInit
    Note right of sdsControlThread: SDS_STATE_INACTIVE 
    loop every 100 ms
      Note over sdsControlThread: sdsExchange
      activate Server
      Server-->>sdsControlThread: SDSIO_CMD_FLAGS: set SDS_FLAG_ALIVE
      sdsControlThread-->>Server: SDSIO_CMD_INFO
    end
    Note right of sdsControlThread: SDS_STATE_CONNECTED 
    deactivate sdsControlThread
    deactivate Server
```

!!! Note
    - When the command `SDS_CMD_FLAGS` sets SDS_FLAG_ALIVE the sdsControlThread transitions into the SDS_STATE_CONNECTED.
    - When `SDSIO_CMD_INFO` is sent more than 10 times without a `SDSIO_CMD_FLAGS` response the sdsControlThread transitions into the SDS_STATE_INACTIVE.

**Recording start flowchart**

```mermaid
sequenceDiagram
    participant sdsControlThread
    activate sdsControlThread
    participant AlgorithmThread
    activate AlgorithmThread
    participant SDSIO as sdsThread
    activate SDSIO
    participant Server as SDSIO-Server<br/>(host)
    activate Server

    Note right of sdsControlThread: SDS_STATE_CONNECTED 

    loop every 100 ms
      Note over sdsControlThread: sdsExchange
      Server->>sdsControlThread: SDSIO_CMD_FLAGS
      sdsControlThread->>Server: SDSIO_CMD_INFO
    end

    Server-->>sdsControlThread: SDSIO_CMD_FLAGS: set SDS_FLAG_START
    Note right of sdsControlThread: SDS_STATE_START

    Note over sdsControlThread: sdsOpen<br/>(all streams)
    sdsControlThread->>Server: SDSIO_CMD_OPEN
    Server->>sdsControlThread: Response

    Note right of sdsControlThread: SDS_STATE_ACTIVE

    loop periodic
        Note over AlgorithmThread: sdsWrite
        AlgorithmThread->>SDSIO: Buffered data reached or crossed threshold
        loop send all data from buffer
            SDSIO->>Server: SDSIO_CMD_WRITE
        end
    end

    deactivate sdsControlThread
    deactivate AlgorithmThread
    deactivate SDSIO
    deactivate Server
```

**Recording stop flowchart**

```mermaid
sequenceDiagram
    participant sdsControlThread
    activate sdsControlThread

    participant AlgorithmThread
    activate AlgorithmThread
    participant SDSIO as sdsThread
    activate SDSIO
    participant Server as SDSIO-Server<br/>(host)
    activate Server

    Note right of sdsControlThread: SDS_STATE_ACTIVE 

    loop every 100 ms
      Note over sdsControlThread: sdsExchange
      Server->>sdsControlThread: SDSIO_CMD_FLAGS
      sdsControlThread->>Server: SDSIO_CMD_INFO
    end

    Server-->>sdsControlThread: SDSIO_CMD_FLAGS: clear SDS_FLAG_START
    Note right of sdsControlThread: SDS_STATE_STOP_REQ
    loop send all data from buffer
        SDSIO->>Server: SDSIO_CMD_WRITE
    end
    Note right of sdsControlThread: SDS_STATE_STOP_DONE

    Note over sdsControlThread: sdsClose<br/>(all streams)
    sdsControlThread->>Server: SDSIO_CMD_CLOSE
    Server->>sdsControlThread: Response
    Note right of sdsControlThread: SDS_STATE_CONNECTED

    deactivate AlgorithmThread
    deactivate SDSIO
    deactivate Server
    deactivate sdsControlThread
```

**Playback flowchart**

```mermaid
sequenceDiagram
    participant sdsControlThread
    activate sdsControlThread
    participant AlgorithmThread
    activate AlgorithmThread
    participant SDSIO as sdsThread
    activate SDSIO
    participant Server as SDSIO-Server<br/>(host)
    activate Server

    Note right of sdsControlThread: SDS_STATE_CONNECTED 

    loop every 100 ms
      Note over sdsControlThread: sdsExchange
      Server->>sdsControlThread: SDSIO_CMD_FLAGS
      sdsControlThread->>Server: SDSIO_CMD_INFO
    end

    Server-->>sdsControlThread: SDSIO_CMD_FLAGS: set SDS_FLAG_START and SDS_FLAG_PLAYBACK
    Note right of sdsControlThread: SDS_STATE_START

    Note over sdsControlThread: sdsOpen<br/>(all streams)
    sdsControlThread->>Server: SDSIO_CMD_OPEN
    Server->>sdsControlThread: Response

    loop read data until threshold is reached
        sdsControlThread->>Server: SDSIO_CMD_READ
        Server->>sdsControlThread: Data
    end
  
    Note right of sdsControlThread: SDS_STATE_ACTIVE

    loop periodic
        Note over AlgorithmThread: sdsRead
        AlgorithmThread->>SDSIO: Buffer data falls below threshold
        loop read data to fill the buffer
            SDSIO->>Server: SDSIO_CMD_READ
            Server-->>SDSIO: Data
        end
    end

    Server-->>SDSIO: No more data

    Server->>AlgorithmThread: End of Stream (EOS)
    Note over AlgorithmThread: sdsRead = SDS_EOS

    AlgorithmThread->>sdsControlThread: End of Stream (EOS)
    Note right of sdsControlThread: SDS_STATE_STOP_REQ
    Note right of sdsControlThread: SDS_STATE_STOP_DONE

    Note over sdsControlThread: sdsClose<br/>(all streams)
    sdsControlThread->>Server: SDSIO_CMD_CLOSE
    Server->>sdsControlThread: Response
    Note right of sdsControlThread: SDS_STATE_CONNECTED

    deactivate sdsControlThread
    deactivate AlgorithmThread
    deactivate SDSIO
    deactivate Server
```
