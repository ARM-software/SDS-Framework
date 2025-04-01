

# File sds\_buffer.h



[**FileList**](files.md) **>** [**include**](dir_d09908635ef304ba819d3349bcb716bf.md) **>** [**sds\_buffer.h**](sds__buffer_8h.md)

[Go to the source code of this file](sds__buffer_8h_source.md)



* `#include <stdint.h>`

















## Public Types

| Type | Name |
| ---: | :--- |
| typedef void(\* | [**sdsBufferEvent\_t**](#typedef-sdsbufferevent_t)  <br>_Call back function for SDS circular buffer handling._  |
| typedef void \* | [**sdsBufferId\_t**](#typedef-sdsbufferid_t)  <br>_Handle to SDS buffer stream._  |




















## Public Functions

| Type | Name |
| ---: | :--- |
|  int32\_t | [**sdsBufferClear**](#function-sdsbufferclear) (sdsBufferId\_t id) <br>_Clear SDS buffer stream data._  |
|  int32\_t | [**sdsBufferClose**](#function-sdsbufferclose) (sdsBufferId\_t id) <br>_Close SDS buffer stream._  |
|  uint32\_t | [**sdsBufferGetCount**](#function-sdsbuffergetcount) (sdsBufferId\_t id) <br>_Get data count in SDS buffer stream._  |
|  sdsBufferId\_t | [**sdsBufferOpen**](#function-sdsbufferopen) (void \* buf, uint32\_t buf\_size, uint32\_t threshold\_low, uint32\_t threshold\_high) <br>_Open SDS buffer stream._  |
|  uint32\_t | [**sdsBufferRead**](#function-sdsbufferread) (sdsBufferId\_t id, void \* buf, uint32\_t buf\_size) <br>_Read data from SDS buffer stream._  |
|  int32\_t | [**sdsBufferRegisterEvents**](#function-sdsbufferregisterevents) (sdsBufferId\_t id, sdsBufferEvent\_t event\_cb, uint32\_t event\_mask, void \* event\_arg) <br> |
|  uint32\_t | [**sdsBufferWrite**](#function-sdsbufferwrite) (sdsBufferId\_t id, const void \* buf, uint32\_t buf\_size) <br>_Write data to SDS buffer stream._  |



























## Macros

| Type | Name |
| ---: | :--- |
| define  | [**SDS\_BUFFER\_ERROR**](sds__buffer_8h.md#define-sds_buffer_error)  `(-1)`<br>_Operation failed._  |
| define  | [**SDS\_BUFFER\_EVENT\_DATA\_HIGH**](sds__buffer_8h.md#define-sds_buffer_event_data_high)  `(1UL &lt;&lt; 1)`<br>_Data above or equal to threshold._  |
| define  | [**SDS\_BUFFER\_EVENT\_DATA\_LOW**](sds__buffer_8h.md#define-sds_buffer_event_data_low)  `(1UL &lt;&lt; 0)`<br>_Events._  |
| define  | [**SDS\_BUFFER\_OK**](sds__buffer_8h.md#define-sds_buffer_ok)  `(0)`<br>_Function return codes._  |

## Public Types Documentation




### typedef sdsBufferEvent\_t 

_Call back function for SDS circular buffer handling._ 
```C++
typedef void(* sdsBufferEvent_t) (sdsBufferId_t id, uint32_t event, void *arg);
```





**Parameters:**


* `id` handle to SDS file for playback 
* `event` event code 
* `arg` pointer to argument 




        

<hr>



### typedef sdsBufferId\_t 

_Handle to SDS buffer stream._ 
```C++
typedef void* sdsBufferId_t;
```




<hr>
## Public Functions Documentation




### function sdsBufferClear 

_Clear SDS buffer stream data._ 
```C++
int32_t sdsBufferClear (
    sdsBufferId_t id
) 
```





**Parameters:**


* `id` sdsBufferId\_t handle to SDS buffer stream 



**Returns:**

return code 





        

<hr>



### function sdsBufferClose 

_Close SDS buffer stream._ 
```C++
int32_t sdsBufferClose (
    sdsBufferId_t id
) 
```





**Parameters:**


* `id` sdsBufferId\_t handle to SDS buffer stream 



**Returns:**

return code 





        

<hr>



### function sdsBufferGetCount 

_Get data count in SDS buffer stream._ 
```C++
uint32_t sdsBufferGetCount (
    sdsBufferId_t id
) 
```





**Parameters:**


* `id` sdsBufferId\_t handle to SDS buffer stream 



**Returns:**

number of bytes in buffer stream 





        

<hr>



### function sdsBufferOpen 

_Open SDS buffer stream._ 
```C++
sdsBufferId_t sdsBufferOpen (
    void * buf,
    uint32_t buf_size,
    uint32_t threshold_low,
    uint32_t threshold_high
) 
```





**Parameters:**


* `buf` pointer to buffer for stream 
* `buf_size` buffer size in bytes 
* `threshold_low` data low threshold in bytes 
* `threshold_high` data high threshold in bytes 



**Returns:**

sdsId\_t Handle to SDS buffer stream 





        

<hr>



### function sdsBufferRead 

_Read data from SDS buffer stream._ 
```C++
uint32_t sdsBufferRead (
    sdsBufferId_t id,
    void * buf,
    uint32_t buf_size
) 
```





**Parameters:**


* `id` sdsBufferId\_t handle to SDS buffer stream 
* `buf` pointer to buffer for data to read 
* `buf_size` buffer size in bytes 



**Returns:**

number of bytes read 





        

<hr>



### function sdsBufferRegisterEvents 

```C++
int32_t sdsBufferRegisterEvents (
    sdsBufferId_t id,
    sdsBufferEvent_t event_cb,
    uint32_t event_mask,
    void * event_arg
) 
```




<hr>



### function sdsBufferWrite 

_Write data to SDS buffer stream._ 
```C++
uint32_t sdsBufferWrite (
    sdsBufferId_t id,
    const void * buf,
    uint32_t buf_size
) 
```





**Parameters:**


* `id` sdsBufferId\_t handle to SDS buffer stream 
* `buf` pointer to buffer with data to write 
* `buf_size` buffer size in bytes 



**Returns:**

number of bytes written 





        

<hr>
## Macro Definition Documentation





### define SDS\_BUFFER\_ERROR 

_Operation failed._ 
```C++
#define SDS_BUFFER_ERROR `(-1)`
```




<hr>



### define SDS\_BUFFER\_EVENT\_DATA\_HIGH 

_Data above or equal to threshold._ 
```C++
#define SDS_BUFFER_EVENT_DATA_HIGH `(1UL << 1)`
```




<hr>



### define SDS\_BUFFER\_EVENT\_DATA\_LOW 

_Events._ 
```C++
#define SDS_BUFFER_EVENT_DATA_LOW `(1UL << 0)`
```



Data bellow or equal to threshold 


        

<hr>



### define SDS\_BUFFER\_OK 

_Function return codes._ 
```C++
#define SDS_BUFFER_OK `(0)`
```



Operation completed successfully 


        

<hr>

------------------------------
The documentation for this class was generated from the following file `sds/include/sds_buffer.h`

