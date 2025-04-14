

# Group SDS\_Recorder\_Player



[**Modules**](modules.md) **>** [**SDS\_Recorder\_Player**](group__SDS__Recorder__Player.md)



sds\_rec\_play.h _: SDS Recorder and Player for writing and reading SDS files via communication or file I/O interface._[More...](#detailed-description)


















## Public Types

| Type | Name |
| ---: | :--- |
| typedef void \* | [**sdsRecPlayId\_t**](#typedef-sdsrecplayid_t)  <br>_Handle to SDS Recorder/Player stream._  |




















## Public Functions

| Type | Name |
| ---: | :--- |
|  int32\_t | [**sdsPlayClose**](#function-sdsplayclose) ([**sdsRecPlayId\_t**](group__SDS__Recorder__Player.md#typedef-sdsrecplayid_t) id) <br>_Close player stream._  |
|  int32\_t | [**sdsPlayEndOfStream**](#function-sdsplayendofstream) ([**sdsRecPlayId\_t**](group__SDS__Recorder__Player.md#typedef-sdsrecplayid_t) id) <br>_Check if end of stream has been reached._  |
|  uint32\_t | [**sdsPlayGetSize**](#function-sdsplaygetsize) ([**sdsRecPlayId\_t**](group__SDS__Recorder__Player.md#typedef-sdsrecplayid_t) id) <br>_Get data block size from Player stream._  |
|  [**sdsRecPlayId\_t**](group__SDS__Recorder__Player.md#typedef-sdsrecplayid_t) | [**sdsPlayOpen**](#function-sdsplayopen) (const char \* name, void \* buf, uint32\_t buf\_size) <br>_Open player stream (read mode)._  |
|  uint32\_t | [**sdsPlayRead**](#function-sdsplayread) ([**sdsRecPlayId\_t**](group__SDS__Recorder__Player.md#typedef-sdsrecplayid_t) id, uint32\_t \* timestamp, void \* buf, uint32\_t buf\_size) <br> |
|  int32\_t | [**sdsRecClose**](#function-sdsrecclose) ([**sdsRecPlayId\_t**](group__SDS__Recorder__Player.md#typedef-sdsrecplayid_t) id) <br>_Close recorder stream._  |
|  [**sdsRecPlayId\_t**](group__SDS__Recorder__Player.md#typedef-sdsrecplayid_t) | [**sdsRecOpen**](#function-sdsrecopen) (const char \* name, void \* buf, uint32\_t buf\_size) <br>_Open recorder stream (write mode)._  |
|  int32\_t | [**sdsRecPlayInit**](#function-sdsrecplayinit) (sdsRecPlayEvent\_t event\_cb) <br>_Initialize recorder and player._  |
|  int32\_t | [**sdsRecPlayUninit**](#function-sdsrecplayuninit) (void) <br>_Uninitialize recorder and player._  |
|  uint32\_t | [**sdsRecWrite**](#function-sdsrecwrite) ([**sdsRecPlayId\_t**](group__SDS__Recorder__Player.md#typedef-sdsrecplayid_t) id, uint32\_t timestamp, const void \* buf, uint32\_t buf\_size) <br>_Write entire data block along with its timestamp to the recorder stream._  |




























## Detailed Description


via a communication stack, file system, or semihosting interface. Refer to the chapter SDS Interface for an overview. 


    
## Public Types Documentation




### typedef sdsRecPlayId\_t 

_Handle to SDS Recorder/Player stream._ 
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



Closes a player stream. The function waits until all transfers are completed or a timeout occurs. The stream handle becomes invalid after successful closing.




**Parameters:**


* `id` [**sdsRecPlayId\_t**](group__SDS__Recorder__Player.md#typedef-sdsrecplayid_t) handle to SDS Recorder/Player stream 



**Returns:**

return code 





        

<hr>



### function sdsPlayEndOfStream 

_Check if end of stream has been reached._ 
```
int32_t sdsPlayEndOfStream (
    sdsRecPlayId_t id
) 
```



Checks whether the player stream has reached the end of the SDS file. This condition is met when the SDS file signals end-of-stream and all buffered data has been read.




**Parameters:**


* `id` [**sdsRecPlayId\_t**](group__SDS__Recorder__Player.md#typedef-sdsrecplayid_t) handle to SDS Recorder/Player stream 



**Returns:**

nonzero if end of stream, else 0 





        

<hr>



### function sdsPlayGetSize 

_Get data block size from Player stream._ 
```
uint32_t sdsPlayGetSize (
    sdsRecPlayId_t id
) 
```



Returns the size of the next available data block in the player stream.


The function reads the record header to determine the size of the next data block. If no valid record header is available the function returns 0.


This function is thread-safe when used with different streams in separate threads. However, accessing the same stream from multiple threads simultaneously is not supported.




**Parameters:**


* `id` [**sdsRecPlayId\_t**](group__SDS__Recorder__Player.md#typedef-sdsrecplayid_t) handle to SDS Recorder/Player stream 



**Returns:**

number of bytes in data block 





        

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



Opens a player stream for reading timestamp and data blocks from the SDS file. The `buf` parameter specifies a user-allocated memory region that serves as an internal circular buffer. The buffer size must be sufficient to accommodate at least the largest data block size plus 8 bytes for the record header.


The `name` parameter specifies the base name of the SDS input file. The function attempts to locate and open the file `<name>.<index>.sds`, where `<index>` is an auto-incrementing value. If no matching file is found, the function returns an error.


Note that this function can be blocking for some time as it initially fills the internal circular buffer with data from the SDS file.


This function returns a handle that uniquely identifies the stream. The handle is used as a reference in subsequent function calls to perform operations on the stream.




**Parameters:**


* `name` stream name (pointer to NULL terminated string) 
* `buf` pointer to buffer for player stream 
* `buf_size` buffer size in bytes 



**Returns:**

[**sdsRecPlayId\_t**](group__SDS__Recorder__Player.md#typedef-sdsrecplayid_t) handle to SDS Recorder/Player stream or NULL if operation failed 





        

<hr>



### function sdsPlayRead 

```
uint32_t sdsPlayRead (
    sdsRecPlayId_t id,
    uint32_t * timestamp,
    void * buf,
    uint32_t buf_size
) 
```



Reads a data block along with its associated timestamp from the internal circular buffer. The `sdsRecPlayThread` worker thread asynchronously reads the data from the SDS file using the underlying SDS I/O interface and writes it to the internal circular buffer. This approach ensures efficient, non-blocking data handling and optimal performance.


Before reading, the function verifies that the user-provided buffer has sufficient space to accommodate the entire data block. If the buffer size is insufficient, the operation is aborted, and the function returns 0. Additionally, if the end-of-stream condition is reached, the function also returns 0. Therefore, it is necessary to call `sdsPlayEndOfStream` to identify whether the end-of-stream condition has been met.


On success, the function reads the data block from the stream buffer, stores it in the user-provided buffer, and returns the number of bytes in the data block. Additionally, the timestamp associated with the data block is provided in the output parameter `timestamp`.


Thread safety is maintained by allowing only one thread to read from an individual stream at a time. However, multiple threads can read from different streams concurrently, enabling parallel operations across multiple streams.


/\*\* 


        

<hr>



### function sdsRecClose 

_Close recorder stream._ 
```
int32_t sdsRecClose (
    sdsRecPlayId_t id
) 
```



Closes a recorder stream. Before closing, any data remaining in the internal circular buffer is written to the SDS file. The function waits until all transfers are completed or a timeout occurs. The stream handle becomes invalid after successful closing.




**Parameters:**


* `id` [**sdsRecPlayId\_t**](group__SDS__Recorder__Player.md#typedef-sdsrecplayid_t) handle to SDS Recorder/Player stream 



**Returns:**

return code 





        

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



Opens a recorder stream for writing timestamps and data blocks to the SDS file. The `buf` parameter specifies a user-allocated memory region that serves as an internal circular buffer. The buffer size must be sufficient to accommodate at least the largest data block size plus 8 bytes for the record header.


The `name` parameter defines the base name for the SDS output file and is used to construct the file name in the format `name.index.sds`. The `index` is an auto-incrementing value that ensures a unique file name is generated. If a file with the specified name already exists, the `index` is incremented until a unique name is found.


This function returns a handle that uniquely identifies the stream. The handle is used as a reference in subsequent function calls to perform operations on the stream.




**Parameters:**


* `name` stream name (pointer to NULL terminated string) 
* `buf` pointer to buffer for recorder stream 
* `buf_size` buffer size in bytes 



**Returns:**

[**sdsRecPlayId\_t**](group__SDS__Recorder__Player.md#typedef-sdsrecplayid_t) handle to SDS Recorder/Player stream or NULL if operation failed 





        

<hr>



### function sdsRecPlayInit 

_Initialize recorder and player._ 
```
int32_t sdsRecPlayInit (
    sdsRecPlayEvent_t event_cb
) 
```



Initializes the SDS Recorder and Player system. This function allocates resources, initializes underlying SDS I/O interface and starts the `sdsRecPlayThread` worker thread. An optional callback function can be registered to receive notifications (e.g., I/O errors). This function must be called once before any recorder or player streams are opened.




**Parameters:**


* `event_cb` pointer to sdsRecPlayEvent\_t callback function 



**Returns:**

return code 





        

<hr>



### function sdsRecPlayUninit 

_Uninitialize recorder and player._ 
```
int32_t sdsRecPlayUninit (
    void
) 
```



De-initializes the SDS Recorder and Player system. The `sdsRecPlayThread` worker thread is stopped, and internal resources are released. All open recorder or player streams must be closed by the user before calling this function. Once uninitialized, the system must be initialized again before use.




**Returns:**

return code 





        

<hr>



### function sdsRecWrite 

_Write entire data block along with its timestamp to the recorder stream._ 
```
uint32_t sdsRecWrite (
    sdsRecPlayId_t id,
    uint32_t timestamp,
    const void * buf,
    uint32_t buf_size
) 
```



Writes a data block with header (timestamp and size of data block) to the internal circular buffer. The `sdsRecPlayThread` worker thread asynchronously reads the data from the internal circular buffer and writes it to the SDS file using the underlying SDS I/O interface. This approach ensures efficient, non-blocking data handling and optimal performance.


Before writing, the function ensures that the data block can fit into the internal buffer. If there is insufficient space, the operation is aborted, and the function returns 0.


On success, the function writes the header and data block to the stream buffer and returns the number of data bytes written (excluding the header).


Thread safety is maintained by allowing only one thread to write to an individual stream at a time. However, multiple threads can write to different streams concurrently, enabling parallel operations across multiple streams.




**Parameters:**


* `id` [**sdsRecPlayId\_t**](group__SDS__Recorder__Player.md#typedef-sdsrecplayid_t) handle to SDS Recorder/Player stream 
* `timestamp` timestamp in ticks 
* `buf` pointer to the data block buffer to be written 
* `buf_size` size of the data block buffer in bytes 



**Returns:**

size of the entire data block written in bytes if the operation is successful, or 0 if the entire data block could not be written successfully 





        

<hr>

------------------------------


