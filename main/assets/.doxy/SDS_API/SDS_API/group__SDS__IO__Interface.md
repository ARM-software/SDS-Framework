

# Group SDS\_IO\_Interface



[**Modules**](modules.md) **>** [**SDS\_IO\_Interface**](group__SDS__IO__Interface.md)



sdsio.h _: SDS I/O Interface for data streams_[More...](#detailed-description)


















## Public Types

| Type | Name |
| ---: | :--- |
| typedef void \* | [**sdsioId\_t**](#typedef-sdsioid_t)  <br>_Handle to SDS I/O stream._  |
| enum  | [**sdsioMode\_t**](#enum-sdsiomode_t)  <br>_Open Mode._  |




















## Public Functions

| Type | Name |
| ---: | :--- |
|  int32\_t | [**sdsioClose**](#function-sdsioclose) ([**sdsioId\_t**](group__SDS__IO__Interface.md#typedef-sdsioid_t) id) <br>_Close I/O stream._  |
|  int32\_t | [**sdsioEndOfStream**](#function-sdsioendofstream) ([**sdsioId\_t**](group__SDS__IO__Interface.md#typedef-sdsioid_t) id) <br>_Check if end of stream has been reached._  |
|  int32\_t | [**sdsioInit**](#function-sdsioinit) (void) <br>_Initialize SDS I/O._  |
|  [**sdsioId\_t**](group__SDS__IO__Interface.md#typedef-sdsioid_t) | [**sdsioOpen**](#function-sdsioopen) (const char \* name, [**sdsioMode\_t**](group__SDS__IO__Interface.md#enum-sdsiomode_t) mode) <br>_Open I/O stream._  |
|  uint32\_t | [**sdsioRead**](#function-sdsioread) ([**sdsioId\_t**](group__SDS__IO__Interface.md#typedef-sdsioid_t) id, void \* buf, uint32\_t buf\_size) <br>_Read data from I/O stream._  |
|  int32\_t | [**sdsioUninit**](#function-sdsiouninit) (void) <br>_Un-initialize SDS I/O._  |
|  uint32\_t | [**sdsioWrite**](#function-sdsiowrite) ([**sdsioId\_t**](group__SDS__IO__Interface.md#typedef-sdsioid_t) id, const void \* buf, uint32\_t buf\_size) <br>_Write data to I/O stream._  |




























## Detailed Description


The SDS I/O interface reads or writes SDS data from a circular buffer. 


    
## Public Types Documentation




### typedef sdsioId\_t 

_Handle to SDS I/O stream._ 
```
typedef void* sdsioId_t;
```



This _pointer_ defines the handle to SDS I/O data stream. It is used to identify a data stream across the different functions. 


        

<hr>



### enum sdsioMode\_t 

_Open Mode._ 
```
enum sdsioMode_t {
    sdsioModeRead = 0,
    sdsioModeWrite = 1
};
```



This _enum_ identifies the _read_ or _write_ mode to SDS I/O data streams. 


        

<hr>
## Public Functions Documentation




### function sdsioClose 

_Close I/O stream._ 
```
int32_t sdsioClose (
    sdsioId_t id
) 
```



todo




**Parameters:**


* `id` [**sdsioId\_t**](group__SDS__IO__Interface.md#typedef-sdsioid_t) handle to SDS I/O stream 



**Returns:**

return code 





        

<hr>



### function sdsioEndOfStream 

_Check if end of stream has been reached._ 
```
int32_t sdsioEndOfStream (
    sdsioId_t id
) 
```



todo




**Parameters:**


* `id` [**sdsioId\_t**](group__SDS__IO__Interface.md#typedef-sdsioid_t) handle to SDS I/O stream 



**Returns:**

nonzero if end of stream, else 0 





        

<hr>



### function sdsioInit 

_Initialize SDS I/O._ 
```
int32_t sdsioInit (
    void
) 
```



todo




**Returns:**

return code 





        

<hr>



### function sdsioOpen 

_Open I/O stream._ 
```
sdsioId_t sdsioOpen (
    const char * name,
    sdsioMode_t mode
) 
```



todo




**Parameters:**


* `name` stream name (pointer to NULL terminated string) 
* `mode` [**sdsioMode\_t**](group__SDS__IO__Interface.md#enum-sdsiomode_t) open mode 



**Returns:**

[**sdsioId\_t**](group__SDS__IO__Interface.md#typedef-sdsioid_t) 





        

<hr>



### function sdsioRead 

_Read data from I/O stream._ 
```
uint32_t sdsioRead (
    sdsioId_t id,
    void * buf,
    uint32_t buf_size
) 
```



todo




**Parameters:**


* `id` [**sdsioId\_t**](group__SDS__IO__Interface.md#typedef-sdsioid_t) handle to SDS I/O stream 
* `buf` pointer to buffer for data to read 
* `buf_size` buffer size in bytes 



**Returns:**

number of bytes read 





        

<hr>



### function sdsioUninit 

_Un-initialize SDS I/O._ 
```
int32_t sdsioUninit (
    void
) 
```



todo




**Returns:**

return code 





        

<hr>



### function sdsioWrite 

_Write data to I/O stream._ 
```
uint32_t sdsioWrite (
    sdsioId_t id,
    const void * buf,
    uint32_t buf_size
) 
```



todo




**Parameters:**


* `id` [**sdsioId\_t**](group__SDS__IO__Interface.md#typedef-sdsioid_t) handle to SDS I/O stream 
* `buf` pointer to buffer with data to write 
* `buf_size` buffer size in bytes 



**Returns:**

number of bytes written 





        

<hr>

------------------------------


