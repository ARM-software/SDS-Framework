

# Group SDS\_IO\_Interface



[**Modules**](modules.md) **>** [**SDS\_IO\_Interface**](group__SDS__IO__Interface.md)



sdsio.h _: SDS I/O Interface for data streams_[More...](#detailed-description)












## Modules

| Type | Name |
| ---: | :--- |
| module | [**Function Return Codes**](group__SDS__IO__Return__Codes.md) <br>_SDS I/O Function Return Codes._  |






## Public Types

| Type | Name |
| ---: | :--- |
| typedef void \* | [**sdsioId\_t**](#typedef-sdsioid_t)  <br>_Handle to SDS I/O data stream._  |
| enum  | [**sdsioMode\_t**](#enum-sdsiomode_t)  <br> |




















## Public Functions

| Type | Name |
| ---: | :--- |
|  int32\_t | [**sdsioClose**](#function-sdsioclose) ([**sdsioId\_t**](group__SDS__IO__Interface.md#typedef-sdsioid_t) id) <br>_Close I/O stream._  |
|  int32\_t | [**sdsioInit**](#function-sdsioinit) (void) <br>_Initialize SDS I/O._  |
|  [**sdsioId\_t**](group__SDS__IO__Interface.md#typedef-sdsioid_t) | [**sdsioOpen**](#function-sdsioopen) (const char \* name, [**sdsioMode\_t**](group__SDS__IO__Interface.md#enum-sdsiomode_t) mode) <br>_Open I/O stream._  |
|  int32\_t | [**sdsioRead**](#function-sdsioread) ([**sdsioId\_t**](group__SDS__IO__Interface.md#typedef-sdsioid_t) id, void \* buf, uint32\_t buf\_size) <br>_Read data from I/O stream._  |
|  int32\_t | [**sdsioUninit**](#function-sdsiouninit) (void) <br>_Un-initialize SDS I/O._  |
|  int32\_t | [**sdsioWrite**](#function-sdsiowrite) ([**sdsioId\_t**](group__SDS__IO__Interface.md#typedef-sdsioid_t) id, const void \* buf, uint32\_t buf\_size) <br>_Write data to I/O stream._  |




























## Detailed Description


The SDS I/O interface provides a generic mechanism for reading from and writing to `SDS file`s using several I/O backends.


Interface can operate over:



* A local file system, such as an SD card or semihosting, where files are accessed directly.
* A communication channel such as Ethernet, USB, or UART, where access to files is performed remotely via an SDS I/O Server.




When using a communication channel, the embedded device runs an SDS I/O Client, which communicates with the SDS I/O Server running on the host machine. This interaction is command-based (e.g., `SDSIO_CMD_OPEN`, `SDSIO_CMD_READ`, `SDSIO_CMD_WRITE`) and enables the embedded system to remotely open, read, write, and close files located on the host. For more details, refer to [SDSIO Server Protocol](../theory.md#sdsio server protocol).


The interface is lightweight and backend-agnostic, making it suitable for embedded data logging, host-interactive tools, or as a transport layer for higher-level components such as the SDS Recorder and Player. 


    
## Public Types Documentation




### typedef sdsioId\_t 

_Handle to SDS I/O data stream._ 
```
typedef void* sdsioId_t;
```



This _pointer_ defines the handle to SDS I/O data stream. It is used to identify a data stream across the different functions. 


        

<hr>



### enum sdsioMode\_t 

```
enum sdsioMode_t {
    sdsioModeRead = 0,
    sdsioModeWrite = 1
};
```



This _enum_ identifies the _read_ or _write_ mode to SDS I/O data streams. It is a parameter of the [**sdsioOpen**](group__SDS__IO__Interface.md#function-sdsioopen) function. 


        

<hr>
## Public Functions Documentation




### function sdsioClose 

_Close I/O stream._ 
```
int32_t sdsioClose (
    sdsioId_t id
) 
```



Closes an SDS I/O stream. If the interface is a local file system or semihosting, the file is closed directly. For communication channels such as Ethernet, USB or USART, the SDS I/O Client sends a close command (SDSIO\_CMD\_CLOSE) to the SDS I/O Server to close the file on the Host system.




**Parameters:**


* `id` [**sdsioId\_t**](group__SDS__IO__Interface.md#typedef-sdsioid_t) handle to SDS I/O stream 



**Returns:**

SDSIO\_OK on success or a negative value on error (see [**Function Return Codes**](group__SDS__IO__Return__Codes.md)) 





        

<hr>



### function sdsioInit 

_Initialize SDS I/O._ 
```
int32_t sdsioInit (
    void
) 
```



Initializes the SDS I/O interface. The interface may be a local file system (e.g., an SD card) or semihosting, or a communication channel such as Ethernet, USB or UART. In the case of a communication channel, the SDS I/O Client is used to interact with the SDS I/O Server running on a Host machine. The initialization process includes setting up the communication interface and verifying that the I/O Server is active on the Host.




**Returns:**

SDSIO\_OK on success or a negative value on error (see [**Function Return Codes**](group__SDS__IO__Return__Codes.md)) 





        

<hr>



### function sdsioOpen 

_Open I/O stream._ 
```
sdsioId_t sdsioOpen (
    const char * name,
    sdsioMode_t mode
) 
```



Opens an SDS I/O stream for reading or writing. If the interface is a local file system or semihosting, the file is opened directly. For communication channels such as Ethernet, USB or USART, the SDS I/O Client sends an open command (SDSIO\_CMD\_OPEN) to the SDS I/O Server to open the file on the Host system. The function returns the handle to the SDS I/O stream; if the I/O stream could not be opened, it returns NULL.




**Parameters:**


* `name` stream name (pointer to NULL terminated string) 
* `mode` [**sdsioMode\_t**](group__SDS__IO__Interface.md#enum-sdsiomode_t) open mode 



**Returns:**

[**sdsioId\_t**](group__SDS__IO__Interface.md#typedef-sdsioid_t) Handle to SDS I/O stream, or NULL if operation failed 





        

<hr>



### function sdsioRead 

_Read data from I/O stream._ 
```
int32_t sdsioRead (
    sdsioId_t id,
    void * buf,
    uint32_t buf_size
) 
```



Attempts to read up to `buf_size` bytes of data from the SDS I/O stream identified by `id` into the memory pointed to `buf`. If the interface is a local file system or semihosting, data is read directly from the file. For communication channels such as Ethernet, USB or USART, the SDS I/O Client sends a read command (SDSIO\_CMD\_READ) to the SDS I/O Server, which reads the file on the Host system and returns the data to the Client.


The function attempts to read data and may block based on the behavior of the underlying interface and data availability. It returns under the following conditions:



* If data is available, the function reads up to `buf_size` bytes and returns the number of bytes actually read. This value may be less than `buf_size`.
* If no data becomes available before the timeout expires, the function returns [**SDSIO\_ERROR\_TIMEOUT**](group__SDS__IO__Return__Codes.md#define-sdsio_error_timeout).
* If data is partially read but the timeout occurs before the full request is satisfied, the function returns the number of bytes read up to that point.
* If the end of the stream is reached and no more data remains, the function returns [**SDSIO\_EOS**](group__SDS__IO__Return__Codes.md#define-sdsio_eos) to indicate that the end of file has been reached and no additional data is available.
* If an I/O interface or protocol error occurs, the function returns [**SDSIO\_ERROR**](group__SDS__IO__Return__Codes.md#define-sdsio_error) or [**SDSIO\_ERROR\_INTERFACE**](group__SDS__IO__Return__Codes.md#define-sdsio_error_interface).






**Parameters:**


* `id` [**sdsioId\_t**](group__SDS__IO__Interface.md#typedef-sdsioid_t) handle to SDS I/O stream 
* `buf` pointer to buffer for data to read 
* `buf_size` buffer size in bytes 



**Returns:**

number of bytes successfully read, or a negative value on error or EOS (see [**Function Return Codes**](group__SDS__IO__Return__Codes.md)) 





        

<hr>



### function sdsioUninit 

_Un-initialize SDS I/O._ 
```
int32_t sdsioUninit (
    void
) 
```



De-initializes the SDS I/O interface. If a communication channel such as Ethernet, USB or USART is used, the corresponding communication interface is also de-initialized.




**Returns:**

SDSIO\_OK on success or a negative value on error (see [**Function Return Codes**](group__SDS__IO__Return__Codes.md)) 





        

<hr>



### function sdsioWrite 

_Write data to I/O stream._ 
```
int32_t sdsioWrite (
    sdsioId_t id,
    const void * buf,
    uint32_t buf_size
) 
```



Attempts to write up to `buf_size` bytes from the memory pointed to `buf` to the SDS I/O stream identified by `id`. If the interface is a local file system or semihosting, data is written directly to the file. For communication channels such as Ethernet, USB or USART, the SDS I/O Client sends a write command (SDSIO\_CMD\_WRITE) along with the data to the SDS I/O Server, which then writes the data to a file on the Host system.


The function may return before all data has been written, depending on the available interface bandwidth, buffer capacity, or timeout behavior:



* If the write operation is successful, the function returns the number of bytes actually written. This value may be less than _buf\_size_ in case of partial write.
* If no data could be written before the operation times out, the function returns [**SDSIO\_ERROR\_TIMEOUT**](group__SDS__IO__Return__Codes.md#define-sdsio_error_timeout).
* If an I/O interface or protocol error occurs, the function returns [**SDSIO\_ERROR**](group__SDS__IO__Return__Codes.md#define-sdsio_error) or [**SDSIO\_ERROR\_INTERFACE**](group__SDS__IO__Return__Codes.md#define-sdsio_error_interface).






**Parameters:**


* `id` [**sdsioId\_t**](group__SDS__IO__Interface.md#typedef-sdsioid_t) handle to SDS I/O stream 
* `buf` pointer to buffer with data to write 
* `buf_size` buffer size in bytes 



**Returns:**

number of bytes successfully written or a negative value on error (see [**Function Return Codes**](group__SDS__IO__Return__Codes.md)) 





        

<hr>

------------------------------


