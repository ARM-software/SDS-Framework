

# Group SDS\_Recorder\_Player



[**Modules**](modules.md) **>** [**SDS\_Recorder\_Player**](group__SDS__Recorder__Player.md)



sds\_rec\_play.h _: SDS Recorder and Player for writing and reading SDS files via communication or file I/O interface._[More...](#detailed-description)












## Modules

| Type | Name |
| ---: | :--- |
| module | [**Event Codes**](group__SDS__Recorder__Player__Event__Codes.md) <br>_SDS Recorder and Player Event Codes._  |
| module | [**Function Return Codes**](group__SDS__Recorder__Player__Return__Codes.md) <br>_SDS Recorder and Player Function Return Codes._  |






## Public Types

| Type | Name |
| ---: | :--- |
| typedef void(\* | [**sdsRecPlayEvent\_t**](#typedef-sdsrecplayevent_t)  <br>_Callback function for recorder and player events._  |
| typedef void \* | [**sdsRecPlayId\_t**](#typedef-sdsrecplayid_t)  <br>_Handle to SDS recorder or player stream._  |




















## Public Functions

| Type | Name |
| ---: | :--- |
|  int32\_t | [**sdsPlayClose**](#function-sdsplayclose) ([**sdsRecPlayId\_t**](group__SDS__Recorder__Player.md#typedef-sdsrecplayid_t) id) <br>_Close player stream._  |
|  int32\_t | [**sdsPlayGetSize**](#function-sdsplaygetsize) ([**sdsRecPlayId\_t**](group__SDS__Recorder__Player.md#typedef-sdsrecplayid_t) id) <br>_Get data block size from Player stream._  |
|  [**sdsRecPlayId\_t**](group__SDS__Recorder__Player.md#typedef-sdsrecplayid_t) | [**sdsPlayOpen**](#function-sdsplayopen) (const char \* name, void \* buf, uint32\_t buf\_size) <br>_Open player stream (read mode)._  |
|  int32\_t | [**sdsPlayRead**](#function-sdsplayread) ([**sdsRecPlayId\_t**](group__SDS__Recorder__Player.md#typedef-sdsrecplayid_t) id, uint32\_t \* timestamp, void \* buf, uint32\_t buf\_size) <br>_Read entire data block along with its timestamp from the player stream._  |
|  int32\_t | [**sdsRecClose**](#function-sdsrecclose) ([**sdsRecPlayId\_t**](group__SDS__Recorder__Player.md#typedef-sdsrecplayid_t) id) <br>_Close recorder stream._  |
|  [**sdsRecPlayId\_t**](group__SDS__Recorder__Player.md#typedef-sdsrecplayid_t) | [**sdsRecOpen**](#function-sdsrecopen) (const char \* name, void \* buf, uint32\_t buf\_size) <br>_Open recorder stream (write mode)._  |
|  int32\_t | [**sdsRecPlayInit**](#function-sdsrecplayinit) ([**sdsRecPlayEvent\_t**](group__SDS__Recorder__Player.md#typedef-sdsrecplayevent_t) event\_cb) <br>_Initialize recorder and player._  |
|  int32\_t | [**sdsRecPlayUninit**](#function-sdsrecplayuninit) (void) <br>_Uninitialize recorder and player._  |
|  int32\_t | [**sdsRecWrite**](#function-sdsrecwrite) ([**sdsRecPlayId\_t**](group__SDS__Recorder__Player.md#typedef-sdsrecplayid_t) id, uint32\_t timestamp, const void \* buf, uint32\_t buf\_size) <br>_Write entire data block along with its timestamp to the recorder stream._  |




























## Detailed Description


The **SDS Recorder** and **Player** manage writing to and reading from SDS files through communication or file I/O interfaces. They support the recording and playback of real-world data for applications such as machine learning and data analysis. For details on I/O interfaces, refer to chapter [SDS Interface](../sdsio.md#).


The system uses a dedicated worker thread (`sdsRecPlayThread`) to handle file I/O asynchronously. User-facing APIs interact only with internal circular buffers, allowing efficient, non-blocking data operations.


Each recorder or player stream is identified by a handle of type [**sdsRecPlayId\_t**](group__SDS__Recorder__Player.md#typedef-sdsrecplayid_t), returned by [**sdsRecOpen**](group__SDS__Recorder__Player.md#function-sdsrecopen) or [**sdsPlayOpen**](group__SDS__Recorder__Player.md#function-sdsplayopen), and is required for all subsequent operations on that stream.


**Thread** **Safety** 


The SDS Recorder and Player are **thread-safe** for regular operation:



* A single thread may read from or write to a specific stream at a time.
* Multiple streams can be used concurrently by separate threads without conflict.




While operational calls are thread-safe, **improper reuse of closed streams can lead to data corruption**:



* When a stream is closed via sdsRecPlayClose, its internal control block may be **reallocated** if another stream is opened.
* If a `read` or `write` operation is still pending on a handle after it has been closed, and a new stream is opened that causes control block reuse, the pending operation may unexpectedly complete on the **newly opened stream**.




To prevent such issues:



* Avoid opening a new stream immediately after closing another unless you can guarantee that all references and asynchronous operations related to the previous stream have been fully completed or canceled. 




    
## Public Types Documentation




### typedef sdsRecPlayEvent\_t 

_Callback function for recorder and player events._ 
```
typedef void(* sdsRecPlayEvent_t) (sdsRecPlayId_t id, uint32_t event);
```



This function is registered by passing a pointer to it as a parameter to the [**sdsRecPlayInit**](group__SDS__Recorder__Player.md#function-sdsrecplayinit) function. It is invoked when an error happens during recording or playback.




**Parameters:**


* `id` [**sdsRecPlayId\_t**](group__SDS__Recorder__Player.md#typedef-sdsrecplayid_t) handle to SDS Recorder/Player stream 
* `event` event code (see [**Event Codes**](group__SDS__Recorder__Player__Event__Codes.md)) 




        

<hr>



### typedef sdsRecPlayId\_t 

_Handle to SDS recorder or player stream._ 
```
typedef void* sdsRecPlayId_t;
```



This _pointer_ defines the handle to SDS recorder or player stream. It is used to identify a data stream across the different functions for the SDS Recorder and Player system. 


        

<hr>
## Public Functions Documentation




### function sdsPlayClose 

_Close player stream._ 
```
int32_t sdsPlayClose (
    sdsRecPlayId_t id
) 
```



Closes a player stream. The function waits for all pending data transfers to complete or until a timeout occurs. Upon successful closure, the stream handle becomes invalid.




**Parameters:**


* `id` [**sdsRecPlayId\_t**](group__SDS__Recorder__Player.md#typedef-sdsrecplayid_t) handle to SDS Recorder/Player stream 



**Returns:**

SDS\_REC\_PLAY\_OK on success or a negative value on error (see [**Function Return Codes**](group__SDS__Recorder__Player__Return__Codes.md)) 





        

<hr>



### function sdsPlayGetSize 

_Get data block size from Player stream._ 
```
int32_t sdsPlayGetSize (
    sdsRecPlayId_t id
) 
```



Function verifies that the entire header and the complete data block specified by the header are both present in the SDS circular buffer. It returns the size, in bytes, of the next available data block in the player stream. If either the header is incomplete or the corresponding data block is not yet fully available, the function returns [**SDS\_PLAY\_ERROR\_NO\_DATA**](group__SDS__Recorder__Player__Return__Codes.md#define-sds_play_error_no_data). If the end of the stream has been reached and no further data is available, the function returns [**SDS\_PLAY\_EOS**](group__SDS__Recorder__Player__Return__Codes.md#define-sds_play_eos).




**Parameters:**


* `id` [**sdsRecPlayId\_t**](group__SDS__Recorder__Player.md#typedef-sdsrecplayid_t) handle to SDS Recorder/Player stream 



**Returns:**

number of bytes in next available data block, or a negative value on error or EOS (see [**Function Return Codes**](group__SDS__Recorder__Player__Return__Codes.md)) 





        

<hr>



### function sdsPlayOpen 

_Open player stream (read mode)._ 
```
sdsRecPlayId_t sdsPlayOpen (
    const char * name,
    void * buf,
    uint32_t buf_size
) 
```



Opens a player stream for reading timestamps and data blocks from an SDS file. The `buf` parameter specifies a user-allocated memory region used as an internal circular buffer. The buffer must be large enough to accommodate the largest expected data block plus 8 bytes for the record header.


The `name` parameter specifies the base name of the SDS input file. The function attempts to locate and open the file `<name>.<index>.sds`, where `<index>` is an auto-incrementing value. If no matching file is found, the function returns an error. For details, refer to [Filenames section](../theory.md#filenames).


This function may block for a period of time while it loads the internal SDS circular buffer with data from the SDS file.


This function returns a handle that uniquely identifies the stream. The handle is used as a reference in subsequent function calls to perform operations on the stream.




**Parameters:**


* `name` stream name (pointer to NULL terminated string) 
* `buf` pointer to buffer for player stream 
* `buf_size` buffer size in bytes 



**Returns:**

[**sdsRecPlayId\_t**](group__SDS__Recorder__Player.md#typedef-sdsrecplayid_t) handle to SDS Recorder/Player stream, or NULL if operation failed 





        

<hr>



### function sdsPlayRead 

_Read entire data block along with its timestamp from the player stream._ 
```
int32_t sdsPlayRead (
    sdsRecPlayId_t id,
    uint32_t * timestamp,
    void * buf,
    uint32_t buf_size
) 
```



Reads a data block along with its associated timestamp from the internal SDS circular buffer.


The `sdsRecPlayThread` worker thread asynchronously reads the data from the SDS file using the underlying SDS I/O interface. For details on how the specific SDS file is selected, refer to [Filenames section](../theory.md#filenames). The retrieved data is then written to the internal SDS circular buffer. This asynchronous design enables efficient, non-blocking data handling and ensures optimal performance.


Before attempting to read, the function verifies that the entire header and the complete data block specified by the header are both present in the SDS circular buffer. If either the header is incomplete or the corresponding data block is not yet fully available, the function aborts and returns [**SDS\_PLAY\_ERROR\_NO\_DATA**](group__SDS__Recorder__Player__Return__Codes.md#define-sds_play_error_no_data).


The function verifies that the user-provided buffer `buf`, with size `buf_size`, is large enough to accommodate the entire data block. If it is too small, the function aborts and returns [**SDS\_REC\_PLAY\_ERROR\_PARAMETER**](group__SDS__Recorder__Player__Return__Codes.md#define-sds_rec_play_error_parameter).


If the end of the stream has been reached and no further data is available, the function returns [**SDS\_PLAY\_EOS**](group__SDS__Recorder__Player__Return__Codes.md#define-sds_play_eos).


On success, the function reads the data block from the circular buffer, stores it in the user-provided buffer, and returns the the size of the data block in bytes. The associated timestamp is returned via the output parameter `timestamp`.


Thread safety is ensured by allowing only a single thread to read from a given stream at a time. However, multiple threads can concurrently read from different streams, enabling parallel operations across multiple streams.




**Parameters:**


* `id` [**sdsRecPlayId\_t**](group__SDS__Recorder__Player.md#typedef-sdsrecplayid_t) handle to SDS Recorder/Player stream 
* `timestamp` pointer to buffer for a timestamp in ticks 
* `buf` pointer to the data block buffer to be read 
* `buf_size` size of the data block buffer in bytes 



**Returns:**

number of bytes successfully read, or a negative value on error or EOS (see [**Function Return Codes**](group__SDS__Recorder__Player__Return__Codes.md)) 





        

<hr>



### function sdsRecClose 

_Close recorder stream._ 
```
int32_t sdsRecClose (
    sdsRecPlayId_t id
) 
```



Closes a recorder stream. Prior to closing, any remaining data in the internal circular buffer is flushed to the SDS file. The function blocks until all data transfers are complete or a timeout occurs. Upon successful closure, the stream handle becomes invalid.




**Parameters:**


* `id` [**sdsRecPlayId\_t**](group__SDS__Recorder__Player.md#typedef-sdsrecplayid_t) handle to SDS Recorder/Player stream 



**Returns:**

SDS\_REC\_PLAY\_OK on success or a negative value on error (see [**Function Return Codes**](group__SDS__Recorder__Player__Return__Codes.md)) 





        

<hr>



### function sdsRecOpen 

_Open recorder stream (write mode)._ 
```
sdsRecPlayId_t sdsRecOpen (
    const char * name,
    void * buf,
    uint32_t buf_size
) 
```



Opens a recorder stream for writing timestamps and data blocks to the SDS file. The `buf` parameter specifies a user-allocated memory region that serves as an internal circular buffer. The buffer must be large enough to hold at least the largest expected data block plus 8 bytes for the record header.


The `name` parameter defines the base name for the SDS output file and is used to construct the full file name in the format `name.index.sds`. The `index` is an auto-incrementing value that ensures a unique file name is generated. If a file with the generated name already exists, the `index` is incremented until an unused name is found. For details, refer to [Filenames section](../theory.md#filenames).


This function returns a handle that uniquely identifies the stream. The handle is used as a reference in subsequent function calls to perform operations on the stream.




**Parameters:**


* `name` stream name (pointer to NULL terminated string) 
* `buf` pointer to buffer for recorder stream 
* `buf_size` buffer size in bytes 



**Returns:**

[**sdsRecPlayId\_t**](group__SDS__Recorder__Player.md#typedef-sdsrecplayid_t) handle to SDS Recorder/Player stream, or NULL if operation failed 





        

<hr>



### function sdsRecPlayInit 

_Initialize recorder and player._ 
```
int32_t sdsRecPlayInit (
    sdsRecPlayEvent_t event_cb
) 
```



Initializes the SDS Recorder and Player system. This function allocates resources, initializes underlying SDS I/O interface and creates the `sdsRecPlayThread` worker thread. An optional callback function can be registered to receive notifications (e.g., I/O errors). This function must be called once before opening any recorder or player streams.




**Parameters:**


* `event_cb` pointer to [**sdsRecPlayEvent\_t**](group__SDS__Recorder__Player.md#typedef-sdsrecplayevent_t) callback function 



**Returns:**

SDS\_REC\_PLAY\_OK on success or a negative value on error (see [**Function Return Codes**](group__SDS__Recorder__Player__Return__Codes.md)) 





        

<hr>



### function sdsRecPlayUninit 

_Uninitialize recorder and player._ 
```
int32_t sdsRecPlayUninit (
    void
) 
```



De-initializes the SDS Recorder and Player system. This function terminates the `sdsRecPlayThread` worker thread, and releases the internal resources. All open recorder or player streams must be closed by the user before calling this function. After de-initialization, the system must be re-initialized before further use.




**Returns:**

SDS\_REC\_PLAY\_OK on success or a negative value on error (see [**Function Return Codes**](group__SDS__Recorder__Player__Return__Codes.md)) 





        

<hr>



### function sdsRecWrite 

_Write entire data block along with its timestamp to the recorder stream._ 
```
int32_t sdsRecWrite (
    sdsRecPlayId_t id,
    uint32_t timestamp,
    const void * buf,
    uint32_t buf_size
) 
```



Writes a data block, including a header containing the timestamp and data block size, to the internal circular buffer.


The `sdsRecPlayThread` worker thread asynchronously writes the data to the SDS file via the underlying SDS I/O interface. For an explanation of how the SDS Recorder selects and names the target SDS file, refer to [Filenames section](../theory.md#filenames). This asynchronous design enables efficient, non-blocking data handling and optimized performance.


Before attempting to write, function verifies that the entire header and the complete data block, provided via the buffer pointer `buf` and its size `buf_size`, can fit within the available space in the internal SDS circular buffer. If insufficient space is available, the operation is aborted and the function returns [**SDS\_REC\_ERROR\_NO\_SPACE**](group__SDS__Recorder__Player__Return__Codes.md#define-sds_rec_error_no_space).


On success, the function writes the header and data block to the SDS circular buffer and returns the number of data bytes written, excluding the header.


Thread safety is ensured by allowing only a single thread to write to a given stream at a time. However, multiple threads can concurrently write to different streams, enabling parallel operations across multiple streams.




**Parameters:**


* `id` [**sdsRecPlayId\_t**](group__SDS__Recorder__Player.md#typedef-sdsrecplayid_t) handle to SDS Recorder/Player stream 
* `timestamp` timestamp in ticks 
* `buf` pointer to the data block buffer to be written 
* `buf_size` size of the data block buffer in bytes 



**Returns:**

number of bytes successfully written or a negative value on error (see [**Function Return Codes**](group__SDS__Recorder__Player__Return__Codes.md)) 





        

<hr>

------------------------------


