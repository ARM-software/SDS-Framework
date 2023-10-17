# Synchronous Data Stream (SDS)

Synchronous Data Stream transfer interfaces for recording real-world data and 
for replay of captured data in Arm Virtual Hardware (AVH) simulation. 
Supports development of ML and DSP applications.

The following interfaces are defined:
- [Synchronous Data Stream using circular buffer](#synchronous-data-stream-using-circular-buffer)
- [Synchronous Data Stream using Input/Output](#synchronous-data-stream-using-inputoutput)
- [Synchronous Data Stream Recorder](#synchronous-data-stream-recorder)
- [Synchronous Data Stream Player](#synchronous-data-stream-player)

## Synchronous Data Stream using circular buffer

Stream data is written to and read from a circular buffer allocated in RAM. Event callback function 
which is triggered on data threshold reached can be registered.  

The API is defined in [sds.h](include/sds.h). It features the following functions:
- `sdsOpen`: Opens a stream with user provided buffer and specified thresholds for data events. 
  It returns the stream identifier which is used in other functions specifying a stream.
- `sdsClose`: Closes the specified stream.
- `sdsRegisterEvents`: Registers an event callback function for the specified stream 
   with event mask and user argument.
- `sdsWrite`: Writes data to the specified stream and returns the number of bytes written (no overflow). 
  Optional callback with event `SDS_EVENT_DATA_HIGH` is executed at the end when number of bytes 
  in the stream is above or equal to the configured threshold.
- `sdsRead`: Reads data from the specified stream and returns the number of bytes read. 
  Optional callback with event `SDS_EVENT_DATA_LOW` is executed at the end when number of bytes 
  in the stream is below or equal to the configured threshold.
- `sdsClear`: Clears data in specified stream.
- `sdsGetCount`: Gets the number of bytes in the stream.

Function calls shall be non-blocking and thread-safe.

The following reference implementation is provided in [sds.c](source/sds.c). It features:
- user configurable number of streams (default: 16 streams)

## Synchronous Data Stream using Input/Output

Stream data is written to an Output device or read from an Input device. Input/Output device
can be a File System, STDIO, Socket, ...  

The API is defined in [sdsio.h](include/sdsio.h). It features the following functions:
- `sdsioInit`: Initializes the Input/Output interface.
- `sdsioUnInit`: Un-initializes the Input/Output interface.
- `sdsioOpen`: Opens a named I/O stream for read or write operation. 
  It returns the I/O stream identifier which is used in other functions specifying an I/O stream.
- `sdsioClose`: Closes the specified I/O stream.
- `sdsioWrite`: Writes data to the specified I/O stream and returns the number of bytes written (no overflow).
- `sdsioRead`: Reads data from the specified I/O stream and returns the number of bytes read.
- `sdsioEndOfStream`: Checks if end of stream has been reached and returns a nonzero value if end of stream.

Function calls are typically blocking and shall be thread-safe.

## Synchronous Data Stream Recorder

Stream data is recorded (written to an Output device). 
Output device can be a File System, STDIO, Socket, ...  

The API is defined in [sds_rec.h](include/sds_rec.h). It features the following functions:
- `sdsRecInit`: Initializes the recorder interface and registers an optional event callback function.
- `sdsRecUnInit`: Un-initializes the recorder interface.
- `sdsRecOpen`: Opens a named recorder stream with user provided buffer and specified threshold to trigger I/O write. 
  It returns the recorder stream identifier which is used in other functions specifying a recorder stream.
- `sdsRecClose`: Closes the specified recorder stream.
- `sdsRecWrite`: Writes a record with data and timestamp to the specified recorder stream 
  and returns the number of data bytes written (no overflow).

The function `sdsRecWrite` call shall be non-blocking where other function calls are typically blocking. 
All function calls except `sdsRecInit/UnInit` shall be thread-safe.

Optional event callback function is executed with event:
- `SDS_REC_EVENT_IO_ERROR`: when an I/O error occurs during writing to the output device.

The following reference implementation is provided in [sds_rec.c](source/sds_rec.c). It features:
- user configurable number of streams (default: 8 streams, max: 30)
- user configurable maximum record size (default: 1024 bytes)
- uses SDS buffer (non-blocking `sdsRecWrite`) and SDSIO
- uses a thread for reading from SDS buffer and writing to blocking SDSIO

## Synchronous Data Stream Player

Stream data is played back (read from an Input device). 
Input device can be a File System, STDIO, Socket, ...  

The API is defined in [sds_play.h](include/sds_play.h). It features the following functions:
- `sdsPlayInit`: Initializes the player interface and registers an optional event callback function.
- `sdsPlayUninit`: Un-initializes the player interface.
- `sdsPlayOpen`: Opens a named player stream with user provided buffer and specified threshold to trigger I/O read. 
  It returns the player stream identifier which is used in other functions specifying a player stream.
- `sdsPlayClose`: Closes the specified player stream.
- `sdsPlayRead`: Reads a record with data and timestamp from the specified player stream 
  and returns the number of data bytes read.
- `sdsPlayGetSize`: Gets the number of data bytes in the record.
- `sdsPlayEndOfStream`: Checks if end of stream has been reached and returns a nonzero value if end of stream.

The function `sdsPlayRead` call shall be non-blocking where other function calls are typically blocking. 
All function calls except `sdsPlayInit/UnInit` shall be thread-safe.

Optional event callback function is executed with event:
- `SDS_PLAY_EVENT_IO_ERROR`: when an I/O error occurs during reading from the input device. 
