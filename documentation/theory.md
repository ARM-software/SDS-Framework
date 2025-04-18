# Theory of Operation

<!-- markdownlint-disable MD013 -->
<!-- markdownlint-disable MD036 -->

The SDS Framework enables to record and playback one or more data streams to an application that is under development as shown in the diagram below. With the SDSIO Interface the data streams are connected to SDS data files. The file storage can be either embedded within the system and access by a file system or external on a host computer and accessed by a communication interface such as Ethernet or USB.

The DSP or ML algorithms that are tested operate on blocks and are executed periodically. This documentation uses these terms:

- **Data Block**: is a set of input or output data which is processed in one step by a DSP or ML compute note.
- **Block size**: is the number of bytes of a data block. 
- **Interval**: is the periodic time interval that the compute node executes.

![SDSIO Interface for Player and Recorder](images/SDS-InOut.png)

The core of the SDS-Framework is a circular buffer handling (`sds_buffer.c/h`) that is controlled by the Recorder/Player interface functions (`sds_rec_play.c/h`). This circular buffer is the queue for the file I/O communication (`sdsio_x.c / sdsio.h`). Using the Recorder/Player functions, the data stream under development may read and write data streams as shown in the diagram above.

![Implementation Files of SDS](images/Theory_of_Operation.png)

## Usage

The following diagram shows the usage of the SDS Recorder and Player functions (executed in `sdsRecPlayThread`).  The `sdsControlThread` controls the overall execution. `AlgorithmThread` is the thread that executes Signal Conditioning (SC) and ML Model.

```mermaid
sequenceDiagram
    participant sdsControlThread
    participant sdsRecPlayThread
    participant AlgorithmThread
    sdsControlThread->>sdsRecPlayThread: sdsRecPlayInit
    sdsControlThread->>sdsRecPlayThread: sdsPlayOpen `SCinput`
    sdsControlThread->>sdsRecPlayThread: sdsRecOpen `SCoutput`
    sdsControlThread->>sdsRecPlayThread: sdsRecOpen `MLoutput`
    sdsControlThread->>AlgorithmThread: Activate Algorithm
    Activate AlgorithmThread
    loop periodic
        sdsRecPlayThread->>AlgorithmThread: sdsPlayRead `SCinput` (with Timestamp)
        Note over AlgorithmThread: Execute Signal Conditioning
        AlgorithmThread->>sdsRecPlayThread: sdsRecWrite `SCoutput`
        Note over AlgorithmThread: Execute ML Model
        AlgorithmThread->>sdsRecPlayThread: sdsRecWrite `MLoutput`
    end
    sdsControlThread->>AlgorithmThread: Deactivate Algorithm
    Deactivate AlgorithmThread
    sdsControlThread->>sdsRecPlayThread: sdsPlayClose `SCinput`
    sdsControlThread->>sdsRecPlayThread: sdsRecClose `SCoutput`
    sdsControlThread->>sdsRecPlayThread: sdsRecClose `MCoutput`
    sdsControlThread->>sdsRecPlayThread: sdsRecPlayUninit
```

## SDS Data Files

Each data stream is stored in a separate SDS data file. In the diagram below `SCinput.0.sds` is the input to Signal Conditioning, `SCoutput.0.sds` is the output of Signal Conditioning, and `MLoutput.0.sds` is the output of the ML Model. Each execution of the algorithm is represented in a data block with a `timestamp`. The `timestamp` allows to correlate the blocks of different streams. In the above example, all blocks of one algorithm execution have the same timestamp value.

![SDS Files](images/SDS-Files.png)

- Each call to the function `sdsRecWrite` writes one data block.
- Each call to the function `sdsPlayRead` reads one data block.

### Filenames

SDS data filenames use the following file format: `<name>.<file-index>.sds`. `<name>` is the base file name of the SDS data file. The `<file-index>` is a sequential number starting from `0`.

- The function `sdsRecOpen` gets the `<name>` as a parameter. When connected to a file system (for example the [SDSIO-Server](utilities.md#sdsio-server)), existing files starting with `<file-index>="0"` are iterated. The first available `<file-index>` that does not exist will be used as filename to record data. For example, if the file `SensorX.10.sds` already exists, the filename `SensorX.11.sds` will be used.

- The function `sdsPlayOpen` gets the `<name>` as a parameter. When connected to a file system (for example using the [SDSIO-Server](utilities.md#sdsio-server)), the first call uses the `<file-index>="0"`. Each call to `sdsPlayOpen` increments the `<file-index>`.  
  An alternative way to control the `<file-index>` is by using an additional file named `<name>.index.txt` . If this file exists, the numerical value it contains is used as the `<file-index>` for the next `sdsPlayOpen` operation. After the file is opened (for example by the [SDSIO-Server](utilities.md#sdsio-server)), the value in `<name>.index.txt` is incremented by 1 in preparation for the next `sdsPlayOpen` call.

### Timestamp

The timestamp is a 32-bit unsigned value and is used for:

- Alignment of different data streams that have the same timestamp value.
- Order of the SDS data files captured during execution.
- Combining multiple SDS file records with the same timestamp value.

The same timestamp connects different SDS file records. It is therefore useful to
use the same timestamp for the recording of one iteration of a DSP or ML algorithm.
In most cases the granularity of an RTOS tick (typically 1ms) is a good choice for a timestamp value.

#### File Format

The **SDS Framework** uses a binary data file format to store the individual data streams. It supports the recording and playback of multiple data streams that may have jitters.  Therefore each stream contains timestamp information that allows to correlate the data streams as it is for example required in a sensor fusion application.

The binary data format (stored in `*.<n>.sds` data files) has a record structure with a variable size. Each record has the following format:

1. **timestamp**: record timestamp in tick-frequency (32-bit unsigned integer, little endian)
2. **data size**: number of data bytes in the record (32-bit unsigned integer, little endian)
3. **binary data**: SDS stream (little endian, no padding) as described with the `*.sds.yml` file.

#### YAML Metadata Format

The content of each data stream may be described in a [YAML](https://en.wikipedia.org/wiki/YAML) metadata file that is created by the user. The following section defines the YAML format of this metadata file. The file `sds.schema.json` is a schema description of the SDS Format Description.

`sds:`                               | Start of the SDS Format Description
:------------------------------------|---------------------------------------------------
&nbsp;&nbsp;&nbsp; `name:`           | Name of the Synchronous Data Stream (SDS)
&nbsp;&nbsp;&nbsp; `description:`    | Additional descriptive text (optional)
&nbsp;&nbsp;&nbsp; `frequency:`      | Capture frequency of the SDS
&nbsp;&nbsp;&nbsp; `tick-frequency:` | Tick frequency of the timestamp value (optional); default: 1000 for 1 milli-second interval
&nbsp;&nbsp;&nbsp; `content:`        | List of values captured (see below)

`content:`                           | List of values captured (in the order of the data file)
:------------------------------------|---------------------------------------------------
`- value:`                           | Name of the value
&nbsp;&nbsp;&nbsp; `type:`           | Data type of the value
&nbsp;&nbsp;&nbsp; `offset:`         | Offset of the value (optional); default: 0
&nbsp;&nbsp;&nbsp; `scale:`          | Scale factor of the value (optional); default: 1.0
&nbsp;&nbsp;&nbsp; `unit:`           | Physical unit of the value (optional); default: no units

**Example**

This example defines a data stream with the name "sensorX" that contains the values of a gyroscope, temperature sensor, and additional raw data (that are not further described).

![image](./images/SDS-Metainfo.png)

The binary data that are coming form this sensors are stored in data files with the following file format: `<sensor-name>.<file-index>.sds`. In this example the files names could be:

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

## Code Example

The following code snippets show the usage of the **Recorder Interface**. In this case an accelerometer data stream is recorded.

```c
// *** variable definitions ***
struct {                          // sensor data stream format
  uint16_t x;
  uint16_t y;
  uint16_t z;
} accelerometer [30];             // number of samples in one data stream record

sdsRecId_t *accel_id,             // data stream id
uint8_t accel_buf[(sizeof(accel_buf)*2)+0x800];      // data stream buffer for circular buffer handling
     :
// *** function calls ***
   sdsRecPlayInit(NULL);          // init SDS Recorder/Player  
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

- the communication interface technology that may impose specific buffer size requirements to maximize data transfer rates.
- the frequency of the algorithm execution. Fast execution speeds may require a larger buffer.

As a guideline, the buffer size should be at least **(2 × block size) + 2 KB**.  
The minimum recommended buffer size is **0x1000 (4 KB)**.

## SDSIO Server Protocol

The SDSIO Server uses a simple protocol for data exchange between a Host computer and the embedded target that integrates an [SDSIO Interface](sds_interface.md).  The protocol assumes that the correct communication to the server is already ensured by the underlying technology (TCP/IP or USB) and therefore no extra check is implemented.

The following conventions describe the **command semantic** used in the following documentation:

Symbol     | Description
:----------|:----------------------
\>         | Prefix indicating the direction: Command from target to Host.
<          | Prefix indicating the direction: Response from Host to target.
WORD       | 32-bit value (low byte first).
****       | The field above has exactly one occurrence.
++++       | The field above has a variable length.

**Commands:**

Commands are sent from the embedded target to the Host computer running the SDSIO Server.

ID  | Name               | Description
:--:|:-------------------|:------------------------
1   | SDSIO_CMD_OPEN     | Open an SDS data file
2   | SDSIO_CMD_CLOSE    | Close an SDS data file
3   | SDSIO_CMD_WRITE    | Write to an SDS data file
4   | SDSIO_CMD_READ     | Read from an SDS data file
5   | SDSIO_CMD_EOS      | End of Stream
6   | SDSIO_CMD_PING     | Ping Server

Each Command starts with a Header (4 Words) and optional data with variable length. Depending on the Command, the SDSIO Server replies with a Response that repeats the Header and delivers additional data.

**SDSIO_CMD_OPEN**

The Command ID=1 **SDSIO_CMD_OPEN** opens an SDS data file on the Host computer.

SDS data filenames use the following file format: `<name>.<file-index>.sds`. `Name` is the base file name of the SDS data file. `Len of Name` is the size of the string in bytes. `<file-index>` is a sequential number starting from `0`.

`Mode` defines `read` (value=0) or `write` (value=1) operation. For `write`, the SDSIO Server inserts the next available `<file-index>` number that does not exist yet (if `Name.3.sds` exists, the server creates `Name.4.sds`). For `read` the server maintains a list of `Names` that where previously used. When a Name was not used before it opens `<file-index>=0`, i.e. `Name.0.sds`.

```txt
| WORD |  WORD  | WORD | WORD *******|++++++|
>  1   |   0    | Mode | Len of Name | Name |
|******|********|******|*************|++++++|
```

The Response ID=1 **SDSIO_CMD_OPEN** provides a `Handle` that is used to identify the file in subsequent commands.

```txt
| WORD |  WORD  | WORD | WORD *******|
<  1   | Handle | Mode | 0           |
|******|********|******|*************|
```

**SDSIO_CMD_CLOSE**

The Command ID=2 **SDSIO_CMD_CLOSE** closes an SDS data file on the Host computer. The `Handle` is the identifier obtained with **SDSIO_CMD_OPEN**. There is no Response from the SDSIO Server on this command.

```txt
| WORD |  WORD  | WORD | WORD |
>  2   | Handle |  0   |  0   |
|******|********|******|******|
```

**SDSIO_CMD_WRITE**

The Command ID=3 **SDSIO_CMD_WRITE** writes data to an SDS data file on the Host computer. The `Handle` is the identifier obtained with **SDSIO_CMD_OPEN**. `Size` is the `Data` size in bytes.  There is no Response from the SDSIO Server on this command.

```txt
| WORD |  WORD  | WORD | WORD |++++++|
>  3   | Handle |  0   | Size | Data |
|******|********|******|******|++++++|
```

**SDSIO_CMD_READ**

The Command ID=4 **SDSIO_CMD_READ** reads data from an SDS data file on the Host computer. The `Handle` is the identifier obtained with **SDSIO_CMD_OPEN**. `Size` are the number of bytes that should be read.

```txt
| WORD |  WORD  | WORD | WORD |
>  4   | Handle | Size |  0   |
|******|********|******|******|
```

The Response ID=4 **SDSIO_CMD_READ** provides the data read from an SDS data file on the HOST computer.
`Size` is the `Data` size in bytes that was read and `Status` with nonzero = end of stream, else 0.

```txt
| WORD |  WORD  |  WORD  | WORD |++++++|
<  4   | Handle | Status | Size | Data |
|******|********|********|******|++++++|
```

**SDSIO_CMD_EOS**

The Command ID=5 **SDSIO_CMD_EOS** checks if the end of file is reached. The `Handle` is the identifier obtained with **SDSIO_CMD_OPEN**.

```txt
| WORD |  WORD  | WORD | WORD |
>  5   | Handle |  0   |  0   |
|******|********|******|******|
```

The Response ID=5 **SDSIO_CMD_EOS** returns the `Status` with nonzero = end of stream, else 0

```txt
| WORD |  WORD  |  WORD  | WORD |
<  5   | Handle | Status |  0   |
|******|********|********|******|
```

**SDSIO_CMD_PING**

The Command ID=6 **SDSIO_CMD_PING** verifies if the Server is active and reachable on the Host.

```txt
| WORD | WORD | WORD | WORD |
>  6   |  0   |  0   |  0   |
|******|******|******|******|
```

The Response ID=6 **SDSIO_CMD_PING** returns the `Status` with nonzero = server active, else 0

```txt
| WORD | WORD |  WORD  | WORD |
<  6   |  0   | Status |  0   |
|******|******|********|******|
```

## SDSIO Message Sequence

This is the message sequence of the SDS DataTest example when connected to MDK-Middleware Ethernet.
It contains the following threads that execute on the target.

- Control: Overall execution Control thread (sdsControlThread)
- Algorithm: Algorithm under test thread (AlgorithmThread)
- Recorder/Playback: SDS Recorder/Playback thread (sdsRecPlayThread)

The Server is the SDSIO Server executing on the target system.


**Recording flowchart**

```mermaid
sequenceDiagram
    participant sdsControlThread
    activate sdsControlThread

    participant AlgorithmThread
    participant Recorder as sdsRecPlayThread
    participant Server as SDSIO Server

    activate Server
    Note over sdsControlThread: sdsRecPlayInit
    sdsControlThread->>Server: SDSIO_CMD_PING
    Server-->>sdsControlThread: Response

    sdsControlThread->>Recorder: Create thread
    activate Recorder

    Note over sdsControlThread: sdsRecOpen
    sdsControlThread->>Server: SDSIO_CMD_OPEN
    Server-->>sdsControlThread: Response

    activate AlgorithmThread
    loop periodic
        Note over AlgorithmThread: sdsRecWrite
        AlgorithmThread->>Recorder: Buffer data reached or crossed threshold
        loop send all data from buffer
            Recorder->>Server: SDSIO_CMD_WRITE
        end
    end
    deactivate AlgorithmThread

    Note over sdsControlThread: sdsRecClose
    sdsControlThread->>Recorder: Close request
    loop send all data from buffer
        Recorder->>Server: SDSIO_CMD_WRITE
    end
    Recorder->>sdsControlThread: Close confirm
    sdsControlThread->>Server: SDSIO_CMD_CLOSE

    Note over sdsControlThread: sdsRecPlayUninit
    sdsControlThread->>Recorder: Terminate thread
    deactivate Recorder

    deactivate Server
    deactivate sdsControlThread
```


**Playback flowchart**

```mermaid
sequenceDiagram
    participant sdsControlThread
    activate sdsControlThread

    participant AlgorithmThread
    participant Playback as sdsRecPlayThread
    participant Server as SDSIO Server

    activate Server
    Note over sdsControlThread: sdsRecPlayInit
    sdsControlThread->>Server: SDSIO_CMD_PING
    Server-->>sdsControlThread: Response

    sdsControlThread->>Playback: Create thread
    activate Playback

    Note over sdsControlThread: sdsPlayOpen
    sdsControlThread->>Server: SDSIO_CMD_OPEN
    Server-->>sdsControlThread: Response
    sdsControlThread->>Playback: Open request
    loop read data until threshold
        Playback->>Server: SDSIO_CMD_READ
        Server-->>Playback: Data
    end
    Playback->>sdsControlThread: Open confirm

    activate AlgorithmThread
    loop periodic
        Note over AlgorithmThread: sdsPlayRead
        AlgorithmThread->>Playback: Buffer data falls below threshold
        loop read data to fill the buffer
            Playback->>Server: SDSIO_CMD_READ
            Server-->>Playback: Data
        end
    end
    deactivate AlgorithmThread

    Note over sdsControlThread: sdsPlayClose
    sdsControlThread->>Server: SDSIO_CMD_CLOSE

    Note over sdsControlThread: sdsRecPlayUninit
    sdsControlThread->>Playback: Terminate thread
    deactivate Playback

    deactivate Server
    deactivate sdsControlThread
```



ToDo:

- document control blocks in sds.c, sds_rec.c, and sds_play.c (comments might be sufficient)

How does Threshold work?

- when Threshold is reach, the write operation writes the whole buffer.  The transport layer (TCP/IP) may need to split this into multiple packs. Should size be optimized for transport layer?
- This writes all buffers https://github.com/ARM-software/SDS-Framework/blob/main/sds/source/sds_rec.c#L157 until empty. When Recorder is same priority as Algorithm, Algorithm might not be executed for quite a while.

sds.c generates detailed events (are they documented?)
but sds_rec.c does not really use this information

- Threshold event is only set when complete write was possible, is this correct? https://github.com/ARM-software/SDS-Framework/blob/main/sds/source/sds_rec.c#L298 
