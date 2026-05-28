

# Group SDSIO\_Interface



[**Modules**](modules.md) **>** [**SDSIO\_Interface**](group__SDSIO__Interface.md)



sdsio.h _: Input/Output Interface for SDS data streams._[More...](#detailed-description)


















## Public Types

| Type | Name |
| ---: | :--- |
| typedef void \* | [**sdsioId\_t**](#typedef-sdsioid_t)  <br>_Handle to SDSIO data stream._  |
| enum  | [**sdsioMode\_t**](#enum-sdsiomode_t)  <br>_SDSIO stream open mode._  |




















## Public Functions

| Type | Name |
| ---: | :--- |
|  int32\_t | [**sdsioClose**](#function-sdsioclose) ([**sdsioId\_t**](group__SDSIO__Interface.md#typedef-sdsioid_t) id) <br>_Close SDSIO stream._  |
|  int32\_t | [**sdsioInit**](#function-sdsioinit) (void) <br>_Initialize SDSIO interface._  |
|  [**sdsioId\_t**](group__SDSIO__Interface.md#typedef-sdsioid_t) | [**sdsioOpen**](#function-sdsioopen) (const char \* name, [**sdsioMode\_t**](group__SDSIO__Interface.md#enum-sdsiomode_t) mode) <br>_Open SDSIO stream._  |
|  int32\_t | [**sdsioRead**](#function-sdsioread) ([**sdsioId\_t**](group__SDSIO__Interface.md#typedef-sdsioid_t) id, void \* buf, uint32\_t buf\_size) <br>_Read data from SDSIO stream._  |
|  int32\_t | [**sdsioUninit**](#function-sdsiouninit) (void) <br>_Un-initialize SDSIO interface._  |
|  int32\_t | [**sdsioWrite**](#function-sdsiowrite) ([**sdsioId\_t**](group__SDSIO__Interface.md#typedef-sdsioid_t) id, const void \* buf, uint32\_t buf\_size) <br>_Write data to SDSIO stream._  |




























## Detailed Description


The SDSIO interface provides a generic mechanism for reading from and writing to `SDS files` using different SDSIO implementations.


An SDSIO can operate over:



* A local file system, such as an SD card or semihosting, where files are accessed directly.
* A communication channel such as Ethernet, USB, or UART, where access to files is performed remotely via an SDSIO-Server.




When using a communication channel, the embedded device runs an SDSIO-Client, which communicates with the SDSIO-Server running on the host machine. This interaction is command-based (e.g., `SDSIO_CMD_OPEN`, `SDSIO_CMD_READ`, `SDSIO_CMD_WRITE`) and enables the embedded system to remotely open, read, write, and close files located on the host. For more details, refer to [SDSIO-Server Firmware Protocol](../theory.md#sdsio-server-firmware-protocol).


The interface is lightweight and backend-agnostic, making it suitable for embedded data logging, host-interactive tools, or as a transport layer for higher-level components. 


    
## Public Types Documentation




### typedef sdsioId\_t 

_Handle to SDSIO data stream._ 
```
typedef void* sdsioId_t;
```



This _pointer_ defines the handle to SDSIO data stream. It is used to identify a data stream across the different functions. 


        

<hr>



### enum sdsioMode\_t 

_SDSIO stream open mode._ 
```
enum sdsioMode_t {
    sdsioModeRead = 0,
    sdsioModeWrite = 1
};
```



This _enum_ identifies the opening mode of an SDSIO stream: read or write. It is a parameter of the [**sdsioOpen**](group__SDSIO__Interface.md#function-sdsioopen) function. 


        

<hr>
## Public Functions Documentation




### function sdsioClose 

_Close SDSIO stream._ 
```
int32_t sdsioClose (
    sdsioId_t id
) 
```



**Description:** 


Closes an SDSIO stream. If the interface is a local file system or semihosting, the file is closed directly. For communication channels such as Ethernet, USB or USART, the SDSIO-Client sends a close command (SDSIO\_CMD\_CLOSE) to the SDSIO-Server to close the file on the host system.




**Parameters:**


* `id` [**sdsioId\_t**](group__SDSIO__Interface.md#typedef-sdsioid_t) handle to SDSIO stream 



**Returns:**

SDS\_OK on success or a negative value on error (see [**Function Return Codes**](group__SDS__Return__Codes.md)) 





        

<hr>



### function sdsioInit 

_Initialize SDSIO interface._ 
```
int32_t sdsioInit (
    void
) 
```



**Description:** 


Initializes the SDSIO interface. The interface may be a local file system (e.g., an SD card) or semihosting, or a communication channel such as Ethernet, USB or UART. In the case of a communication channel, the SDSIO-Client is used to interact with the SDSIO-Server running on a host machine. The initialization process includes setting up the communication interface and verifying that the SDSIO-Server is active on the host.




**Returns:**

SDS\_OK on success or a negative value on error (see [**Function Return Codes**](group__SDS__Return__Codes.md)) 





        

<hr>



### function sdsioOpen 

_Open SDSIO stream._ 
```
sdsioId_t sdsioOpen (
    const char * name,
    sdsioMode_t mode
) 
```



**Description:** 


Opens an SDSIO stream for reading or writing. If the interface is a local file system or semihosting, the file is opened directly. For communication channels such as Ethernet, USB or USART, the SDSIO-Client sends an open command (SDSIO\_CMD\_OPEN) to the SDSIO-Server to open the file on the host system. The function returns the handle to the SDSIO stream; if the SDSIO stream could not be opened, it returns NULL.




**Parameters:**


* `name` stream name (pointer to NULL terminated string) 
* `mode` [**sdsioMode\_t**](group__SDSIO__Interface.md#enum-sdsiomode_t) open mode 



**Returns:**

[**sdsioId\_t**](group__SDSIO__Interface.md#typedef-sdsioid_t) Handle to SDSIO stream, or NULL if operation failed 





        

<hr>



### function sdsioRead 

_Read data from SDSIO stream._ 
```
int32_t sdsioRead (
    sdsioId_t id,
    void * buf,
    uint32_t buf_size
) 
```



**Description:** 


Attempts to read up to `buf_size` bytes of data from the SDSIO stream identified by `id` into the memory pointed to `buf`. If the interface is a local file system or semihosting, data is read directly from the file. For communication channels such as Ethernet, USB or USART, the SDSIO-Client sends a read command (SDSIO\_CMD\_READ) to the SDSIO-Server, which reads the file on the host system and returns the data to the SDSIO-Client.


The function attempts to read data and may block based on the behavior of the underlying interface and data availability. It returns under the following conditions:



* If data is available, the function reads up to `buf_size` bytes and returns the number of bytes actually read. This value may be less than `buf_size`.
* If no data becomes available before the timeout expires, the function returns [**SDS\_ERROR\_TIMEOUT**](group__SDS__Return__Codes.md#define-sds_error_timeout).
* If data is partially read but the timeout occurs before the full request is satisfied, the function returns the number of bytes read up to that point.
* If the end of the stream is reached and no more data remains, the function returns [**SDS\_EOS**](group__SDS__Return__Codes.md#define-sds_eos) to indicate that the end of file has been reached and no additional data is available.
* If an SDSIO interface or protocol error occurs, the function returns [**SDS\_ERROR\_IO**](group__SDS__Return__Codes.md#define-sds_error_io).






**Parameters:**


* `id` [**sdsioId\_t**](group__SDSIO__Interface.md#typedef-sdsioid_t) handle to SDSIO stream 
* `buf` pointer to buffer for data to read 
* `buf_size` buffer size in bytes 



**Returns:**

number of bytes successfully read, or a negative value on error or SDS\_EOS (see [**Function Return Codes**](group__SDS__Return__Codes.md)) 





        

<hr>



### function sdsioUninit 

_Un-initialize SDSIO interface._ 
```
int32_t sdsioUninit (
    void
) 
```



**Description:** 


De-initializes the SDSIO interface. If a communication channel such as Ethernet, USB or USART is used, the corresponding communication interface is also de-initialized.




**Returns:**

SDS\_OK on success or a negative value on error (see [**Function Return Codes**](group__SDS__Return__Codes.md)) 





        

<hr>



### function sdsioWrite 

_Write data to SDSIO stream._ 
```
int32_t sdsioWrite (
    sdsioId_t id,
    const void * buf,
    uint32_t buf_size
) 
```



**Description:** 


Attempts to write up to `buf_size` bytes from the memory pointed to `buf` to the SDSIO stream identified by `id`. If the interface is a local file system or semihosting, data is written directly to the file. For communication channels such as Ethernet, USB or USART, the SDSIO-Client sends a write command (SDSIO\_CMD\_WRITE) along with the data to the SDSIO-Server, which then writes the data to a file on the host system.


The function may return before all data has been written, depending on the available interface bandwidth, buffer capacity, or timeout behavior:



* If the write operation is successful, the function returns the number of bytes actually written. This value may be less than _buf\_size_ in case of partial write.
* If no data could be written before the operation times out, the function returns [**SDS\_ERROR\_TIMEOUT**](group__SDS__Return__Codes.md#define-sds_error_timeout).
* If an SDIO interface or protocol error occurs, the function returns [**SDS\_ERROR\_IO**](group__SDS__Return__Codes.md#define-sds_error_io).






**Parameters:**


* `id` [**sdsioId\_t**](group__SDSIO__Interface.md#typedef-sdsioid_t) handle to SDSIO stream 
* `buf` pointer to buffer with data to write 
* `buf_size` buffer size in bytes 



**Returns:**

number of bytes successfully written or a negative value on error (see [**Function Return Codes**](group__SDS__Return__Codes.md)) 





        

<hr>

------------------------------


