

# Group SDS\_Interface



[**Modules**](modules.md) **>** [**SDS\_Interface**](group__SDS__Interface.md)



sds.h _: Synchronous Data Stream Interface for writing and reading SDS files via communication or file I/O interface._[More...](#detailed-description)












## Modules

| Type | Name |
| ---: | :--- |
| module | [**Event Codes**](group__SDS__Event__Codes.md) <br>_SDS Event Codes._  |
| module | [**sdsFlags Bitmasks**](group__SDS__Flag__Masks.md) <br>_SDS Flag Bitmasks._  |
| module | [**Function Return Codes**](group__SDS__Return__Codes.md) <br>_SDS Function Return Codes._  |
| module | [**sdsState Codes**](group__SDS__State__Codes.md) <br>_SDS State Codes._  |






## Public Types

| Type | Name |
| ---: | :--- |
| typedef void(\* | [**sdsEvent\_t**](#typedef-sdsevent_t)  <br>_Callback function for SDS stream events._  |
| typedef void \* | [**sdsId\_t**](#typedef-sdsid_t)  <br>_Handle to SDS stream._  |
| enum  | [**sdsMode\_t**](#enum-sdsmode_t)  <br>_SDS stream open mode._  |




## Public Attributes

| Type | Name |
| ---: | :--- |
|  [**sdsError\_t**](structsdsError__t.md) | [**sdsError**](#variable-sdserror)  <br>_Error information._  |
|  volatile uint32\_t | [**sdsFlags**](#variable-sdsflags)  <br>_Configuration options and control information._  |
|  volatile uint32\_t | [**sdsIdleRate**](#variable-sdsidlerate)  <br>_Idle rate information._  |
|  volatile uint32\_t | [**sdsState**](#variable-sdsstate)  <br>_State information._  |
















## Public Functions

| Type | Name |
| ---: | :--- |
|  int32\_t | [**sdsClose**](#function-sdsclose) ([**sdsId\_t**](group__SDS__Interface.md#typedef-sdsid_t) id) <br>_Close SDS stream._  |
|  int32\_t | [**sdsExchange**](#function-sdsexchange) (void) <br>_Exchange information with the host._  |
|  void | [**sdsFlagsModify**](#function-sdsflagsmodify) (uint32\_t set\_mask, uint32\_t clear\_mask) <br>_Modify_ [_**sdsFlags**_](group__SDS__Interface.md#variable-sdsflags) _control flags (atomic operation)._ |
|  int32\_t | [**sdsGetSize**](#function-sdsgetsize) ([**sdsId\_t**](group__SDS__Interface.md#typedef-sdsid_t) id) <br>_Get data block size from an SDS stream opened in read mode._  |
|  int32\_t | [**sdsInit**](#function-sdsinit) ([**sdsEvent\_t**](group__SDS__Interface.md#typedef-sdsevent_t) event\_cb) <br>_Initialize SDS._  |
|  [**sdsId\_t**](group__SDS__Interface.md#typedef-sdsid_t) | [**sdsOpen**](#function-sdsopen) (const char \* name, [**sdsMode\_t**](group__SDS__Interface.md#enum-sdsmode_t) mode, void \* buf, uint32\_t buf\_size) <br>_Open SDS stream._  |
|  int32\_t | [**sdsRead**](#function-sdsread) ([**sdsId\_t**](group__SDS__Interface.md#typedef-sdsid_t) id, uint32\_t \* timeslot, void \* buf, uint32\_t buf\_size) <br>_Read entire data block along with its timeslot information from the SDS stream opened in read mode._  |
|  int32\_t | [**sdsUninit**](#function-sdsuninit) (void) <br>_Uninitialize SDS._  |
|  int32\_t | [**sdsWrite**](#function-sdswrite) ([**sdsId\_t**](group__SDS__Interface.md#typedef-sdsid_t) id, uint32\_t timeslot, const void \* buf, uint32\_t buf\_size) <br>_Write entire data block along with its timeslot information to the SDS stream opened in write mode._  |




























## Detailed Description


The **SDS** system manages writing to and reading from SDS files through communication or file I/O interfaces. It supports recording and playback of real-world data for applications such as machine learning and data analysis. For details on I/O interfaces, refer to chapter [SDS IO Interface](../sdsio.md#).


The system uses a dedicated worker thread (`sdsThread`) to handle file I/O asynchronously. User-facing APIs interact only with internal circular buffers, allowing efficient, non-blocking data operations.


Each SDS stream is identified by a handle of type [**sdsId\_t**](group__SDS__Interface.md#typedef-sdsid_t), returned by [**sdsOpen**](group__SDS__Interface.md#function-sdsopen) function, and is required for all subsequent operations on that stream.


**Thread** **Safety** 


The SDS Interface functions are **thread-safe** for regular operation:



* A single thread may read from or write to a specific stream at a time.
* Multiple streams can be used concurrently by separate threads without conflict.




While operational calls are thread-safe, **improper reuse of closed streams can lead to data corruption**:



* When a stream is closed via [**sdsClose**](group__SDS__Interface.md#function-sdsclose), its internal control block may be **reallocated** if another stream is opened.
* If a `read` or `write` operation is still pending on a handle after it has been closed, and a new stream is opened that causes control block reuse, the pending operation may unexpectedly complete on the **newly opened stream**.




To prevent such issues:



* Avoid opening a new stream immediately after closing another unless you can guarantee that all references and asynchronous operations related to the previous stream have been fully completed or canceled. 




    
## Public Types Documentation




### typedef sdsEvent\_t 

_Callback function for SDS stream events._ 
```
typedef void(* sdsEvent_t) (sdsId_t id, uint32_t event);
```





**Parameters:**


* `id` [**sdsId\_t**](group__SDS__Interface.md#typedef-sdsid_t) handle of SDS stream 
* `event` event code (see [**Event Codes**](group__SDS__Event__Codes.md))

This callback function is registered by passing a pointer to it as a parameter to the [**sdsInit**](group__SDS__Interface.md#function-sdsinit) function. It is triggered when an event such as insufficient data or space, or an error, occurs during the stream operation. 


        

<hr>



### typedef sdsId\_t 

_Handle to SDS stream._ 
```
typedef void* sdsId_t;
```



This _pointer_ defines the handle to an SDS stream. It is used to identify a data stream across the different functions for the SDS system. 


        

<hr>



### enum sdsMode\_t 

_SDS stream open mode._ 
```
enum sdsMode_t {
    sdsModeRead = 0,
    sdsModeWrite = 1
};
```



This _enum_ identifies the opening mode of an SDS stream: read or write. It is a parameter of the [**sdsOpen**](group__SDS__Interface.md#function-sdsopen) function. 


        

<hr>
## Public Attributes Documentation




### variable sdsError 

_Error information._ 
```
sdsError_t sdsError;
```



This _global structure_ stores SDS diagnostic information, including status code, source file, line number, and an occurrence flag.


This information is relayed to the host in real time as it occurs (see [**sdsExchange**](group__SDS__Interface.md#function-sdsexchange)). 


        

<hr>



### variable sdsFlags 

_Configuration options and control information._ 
```
volatile uint32_t sdsFlags;
```



The sdsFlags are used to store configuration and diagnostic information in a single word. The sdsFlags value can be modified by the SDS application (using the function [**sdsFlagsModify**](group__SDS__Interface.md#function-sdsflagsmodify)) or by the SDSIO-Server.


The sdsFlags are a 32-bit _global variable_ that contains a set of bits used for firmware control and feedback. The highest 8 bits are used by the SDS system (see [**sdsFlags Bitmasks**](group__SDS__Flag__Masks.md)) for control, and the lower 24 bits are available to the user. The user bits are useful for configuring algorithms in A/B tests (i.e. bypass a filter), but can be also used to communicate status information to the host.


The host that runs the SDSIO-Server can update the sdsFlags on demand. The sdsFlags value is also sent to the host periodically (see [**sdsExchange**](group__SDS__Interface.md#function-sdsexchange)). 


        

<hr>



### variable sdsIdleRate 

_Idle rate information._ 
```
volatile uint32_t sdsIdleRate;
```



This _global variable_ stores current value of idle rate in percents, value 0xFFFFFFFF is reserved for unknown value in case that idle time measurement is not available.


This information is relayed to the host in real time periodically (see [**sdsExchange**](group__SDS__Interface.md#function-sdsexchange)). 


        

<hr>



### variable sdsState 

_State information._ 
```
volatile uint32_t sdsState;
```



This _global variable_ contains current state code (see [**sdsState Codes**](group__SDS__State__Codes.md)) of the firmware application.


This information is not relayed to the host; it is available only in the firmware. 


        

<hr>
## Public Functions Documentation




### function sdsClose 

_Close SDS stream._ 
```
int32_t sdsClose (
    sdsId_t id
) 
```





**Parameters:**


* `id` [**sdsId\_t**](group__SDS__Interface.md#typedef-sdsid_t) handle to SDS stream 



**Returns:**

SDS\_OK on success or a negative value on error (see [**Function Return Codes**](group__SDS__Return__Codes.md))


Closes an SDS stream and releases any internal resources associated with the stream handle.


For closing a stream used in **write mode**:



* Prior to closing, any remaining data in the internal circular buffer is flushed to the SDS file. The function blocks until all data transfers are complete or a timeout occurs.




For closing a stream used in **read mode**:



* The function waits for all pending data transfers to complete or until a timeout occurs.




Upon successful closure, the stream handle becomes invalid. 


        

<hr>



### function sdsExchange 

_Exchange information with the host._ 
```
int32_t sdsExchange (
    void
) 
```





**Returns:**

SDS\_OK on success or a negative value on error (see [**Function Return Codes**](group__SDS__Return__Codes.md))


Exchanges SDS control information with the host. Update [**sdsFlags**](group__SDS__Interface.md#variable-sdsflags) if requested by the host, and send current sdsFlags value along with [**sdsIdleRate**](group__SDS__Interface.md#variable-sdsidlerate) and optional error information [**sdsError**](group__SDS__Interface.md#variable-sdserror) to the host. 


        

<hr>



### function sdsFlagsModify 

_Modify_ [_**sdsFlags**_](group__SDS__Interface.md#variable-sdsflags) _control flags (atomic operation)._
```
void sdsFlagsModify (
    uint32_t set_mask,
    uint32_t clear_mask
) 
```





**Parameters:**


* `set_mask` bits to set in sdsFlags (see [**sdsFlags Bitmasks**](group__SDS__Flag__Masks.md)) 
* `clear_mask` bits to clear in sdsFlags (see [**sdsFlags Bitmasks**](group__SDS__Flag__Masks.md))

Atomically sets and clears bits in the global `sdsFlags` control flags. Bits present in `set_mask` are set and bits present in `clear_mask` are cleared in the update, applied in that order. 


        

<hr>



### function sdsGetSize 

_Get data block size from an SDS stream opened in read mode._ 
```
int32_t sdsGetSize (
    sdsId_t id
) 
```





**Parameters:**


* `id` [**sdsId\_t**](group__SDS__Interface.md#typedef-sdsid_t) handle to stream 



**Returns:**

number of bytes in next available data block, or a negative value on error or SDS\_EOS (see [**Function Return Codes**](group__SDS__Return__Codes.md))


Function verifies that the entire header and the complete data block specified by the header are both present in the SDS circular buffer. It returns the size, in bytes, of the next available data block in the stream. If either the header is incomplete or the corresponding data block is not yet fully available, the function returns [**SDS\_NO\_DATA**](group__SDS__Return__Codes.md#define-sds_no_data). If the end of the stream has been reached and no further data is available, the function returns [**SDS\_EOS**](group__SDS__Return__Codes.md#define-sds_eos). 


        

<hr>



### function sdsInit 

_Initialize SDS._ 
```
int32_t sdsInit (
    sdsEvent_t event_cb
) 
```





**Parameters:**


* `event_cb` pointer to [**sdsEvent\_t**](group__SDS__Interface.md#typedef-sdsevent_t) callback function 



**Returns:**

SDS\_OK on success or a negative value on error (see [**Function Return Codes**](group__SDS__Return__Codes.md))


Initializes the SDS system. This function allocates resources, initializes underlying SDS I/O interface and creates the `sdsThread` worker thread. An optional callback function can be registered to receive notifications (e.g., I/O errors). This function must be called once before opening SDS streams. 


        

<hr>



### function sdsOpen 

_Open SDS stream._ 
```
sdsId_t sdsOpen (
    const char * name,
    sdsMode_t mode,
    void * buf,
    uint32_t buf_size
) 
```





**Parameters:**


* `name` SDS stream name (pointer to NULL terminated string) 
* `mode` SDS stream opening mode (see [**sdsMode\_t**](group__SDS__Interface.md#enum-sdsmode_t)) 
* `buf` pointer to buffer for SDS stream 
* `buf_size` buffer size in bytes 



**Returns:**

[**sdsId\_t**](group__SDS__Interface.md#typedef-sdsid_t) handle to SDS stream, or NULL if operation failed


Opens an SDS stream for reading or writing timeslot information and data blocks to or from the SDS file. The `buf` parameter specifies a user-allocated memory region that serves as an internal circular buffer. The buffer must be large enough to hold at least the largest expected data block plus 8 bytes for header information.


For opening stream in **write mode**:



* The `name` parameter defines the base name for the SDS output file and is used to construct the full filename in the format `name.index.sds`. The `index` is an auto-incrementing value that ensures a unique filename is generated. If a file with the generated name already exists, the `index` is incremented until an unused name is found.




For opening stream in **read mode**:



* The `name` parameter specifies the base name of the SDS input file. When SDS I/O is a file system then the function attempts to locate and open the file in the format `name.index.sds`, where `index` is an auto-incrementing value. If no matching file is found, the function returns an error. When SDS I/O uses SDS I/O Client in conjunction with the SDS I/O Server, the filename is constructed by the SDS I/O Server according to configuration specified in the `sdsio.yml` steering file.




For details, refer to [Filenames section](../theory.md#filenames).


This function returns a handle that uniquely identifies the stream. The handle is used as a reference in subsequent function calls to perform operations on the stream. 


        

<hr>



### function sdsRead 

_Read entire data block along with its timeslot information from the SDS stream opened in read mode._ 
```
int32_t sdsRead (
    sdsId_t id,
    uint32_t * timeslot,
    void * buf,
    uint32_t buf_size
) 
```





**Parameters:**


* `id` [**sdsId\_t**](group__SDS__Interface.md#typedef-sdsid_t) handle to SDS stream 
* `timeslot` pointer to buffer for a timeslot value 
* `buf` pointer to the data block buffer to be read 
* `buf_size` size of the data block buffer in bytes 



**Returns:**

number of bytes successfully read, or a negative value on error or SDS\_EOS (see [**Function Return Codes**](group__SDS__Return__Codes.md))


Reads a data block along with its associated timeslot from the internal SDS circular buffer.


The `sdsThread` worker thread asynchronously reads the data from the SDS file using the underlying SDS I/O interface. For details on how the specific SDS file is selected, refer to [Filenames section](../theory.md#filenames). The retrieved data is then written to the internal SDS circular buffer. This asynchronous design enables efficient, non-blocking data handling and ensures optimal performance.


Before attempting to read, the function verifies that the entire header and the complete data block specified by the header are both present in the SDS circular buffer. If either the header is incomplete or the corresponding data block is not yet fully available, the function aborts and returns [**SDS\_NO\_DATA**](group__SDS__Return__Codes.md#define-sds_no_data).


The function verifies that the user-provided buffer `buf`, with size `buf_size`, is large enough to accommodate the entire data block. If it is too small, the function aborts and returns [**SDS\_ERROR\_PARAMETER**](group__SDS__Return__Codes.md#define-sds_error_parameter).


If the end of the stream has been reached and no further data is available, the function returns [**SDS\_EOS**](group__SDS__Return__Codes.md#define-sds_eos).


On success, the function reads the data block from the circular buffer, stores it in the user-provided buffer, and returns the size of the data block in bytes. The associated timeslot is returned via the output parameter `timeslot`.


Thread safety is ensured by allowing only a single thread to read from a given stream at a time. However, multiple threads can concurrently read from different streams, enabling parallel operations across multiple streams. 


        

<hr>



### function sdsUninit 

_Uninitialize SDS._ 
```
int32_t sdsUninit (
    void
) 
```





**Returns:**

SDS\_OK on success or a negative value on error (see [**Function Return Codes**](group__SDS__Return__Codes.md))


De-initializes the SDS system. This function terminates the `sdsThread` worker thread, and releases the internal resources. All open SDS streams must be closed by the user before calling this function. After de-initialization, the system must be re-initialized before further use. 


        

<hr>



### function sdsWrite 

_Write entire data block along with its timeslot information to the SDS stream opened in write mode._ 
```
int32_t sdsWrite (
    sdsId_t id,
    uint32_t timeslot,
    const void * buf,
    uint32_t buf_size
) 
```





**Parameters:**


* `id` [**sdsId\_t**](group__SDS__Interface.md#typedef-sdsid_t) handle to SDS stream 
* `timeslot` timeslot 
* `buf` pointer to the data block buffer to be written 
* `buf_size` size of the data block buffer in bytes 



**Returns:**

number of bytes successfully written or a negative value on error (see [**Function Return Codes**](group__SDS__Return__Codes.md))


Writes a data block, including a header containing the timeslot information and data block size, to the internal circular buffer.


The `sdsThread` worker thread asynchronously writes the data to the SDS file via the underlying SDS I/O interface. For an explanation of how the SDS system selects and names the target SDS file, refer to [Filenames section](../theory.md#filenames). This asynchronous design enables efficient, non-blocking data handling and optimized performance.


Before attempting to write, function verifies that the entire header and the complete data block, provided via the buffer pointer `buf` and its size `buf_size`, can fit within the available space in the internal SDS circular buffer. If insufficient space is available, the operation is aborted and the function returns [**SDS\_NO\_SPACE**](group__SDS__Return__Codes.md#define-sds_no_space).


On success, the function writes the header and data block to the SDS circular buffer and returns the number of data bytes written, excluding the header.


Thread safety is ensured by allowing only a single thread to write to a given stream at a time. However, multiple threads can concurrently write to different streams, enabling parallel operations across multiple streams. 


        

<hr>

------------------------------


