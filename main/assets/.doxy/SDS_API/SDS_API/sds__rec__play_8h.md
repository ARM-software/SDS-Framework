

# File sds\_rec\_play.h



[**FileList**](files.md) **>** [**include**](dir_d09908635ef304ba819d3349bcb716bf.md) **>** [**sds\_rec\_play.h**](sds__rec__play_8h.md)

[Go to the source code of this file](sds__rec__play_8h_source.md)



* `#include <stdint.h>`

















## Public Types

| Type | Name |
| ---: | :--- |
| typedef void(\* | [**sdsRecPlayEvent\_t**](#typedef-sdsrecplayevent_t)  <br>_Call back function for recorder and player events._  |
| typedef void \* | [**sdsRecPlayId\_t**](#typedef-sdsrecplayid_t)  <br>_handle to SDS Recorder/Player stream_  |




















## Public Functions

| Type | Name |
| ---: | :--- |
|  int32\_t | [**sdsPlayClose**](#function-sdsplayclose) (sdsRecPlayId\_t id) <br>_Close player stream._  |
|  int32\_t | [**sdsPlayEndOfStream**](#function-sdsplayendofstream) (sdsRecPlayId\_t id) <br>_Check if end of stream has been reached._  |
|  uint32\_t | [**sdsPlayGetSize**](#function-sdsplaygetsize) (sdsRecPlayId\_t id) <br>_Get record data size from Player stream._  |
|  uint32\_t | [**sdsPlayRead**](#function-sdsplayread) (sdsRecPlayId\_t id, uint32\_t \* timestamp, void \* buf, uint32\_t buf\_size) <br> |
|  int32\_t | [**sdsRecClose**](#function-sdsrecclose) (sdsRecPlayId\_t id) <br>_Close recorder stream._  |
|  int32\_t | [**sdsRecPlayInit**](#function-sdsrecplayinit) (sdsRecPlayEvent\_t event\_cb) <br>_Initialize recorder and player._  |
|  int32\_t | [**sdsRecPlayUninit**](#function-sdsrecplayuninit) (void) <br>_Uninitialize recorder and player._  |
|  uint32\_t | [**sdsRecWrite**](#function-sdsrecwrite) (sdsRecPlayId\_t id, uint32\_t timestamp, const void \* buf, uint32\_t buf\_size) <br> |



























## Macros

| Type | Name |
| ---: | :--- |
| define  | [**SDS\_REC\_PLAY\_ERROR**](sds__rec__play_8h.md#define-sds_rec_play_error)  `(-1)`<br>_Operation failed._  |
| define  | [**SDS\_REC\_PLAY\_EVENT\_IO\_ERROR**](sds__rec__play_8h.md#define-sds_rec_play_event_io_error)  `(1UL &lt;&lt; 0)`<br>_Event codes._  |
| define  | [**SDS\_REC\_PLAY\_OK**](sds__rec__play_8h.md#define-sds_rec_play_ok)  `(0)`<br>_Function return codes._  |

## Public Types Documentation




### typedef sdsRecPlayEvent\_t 

_Call back function for recorder and player events._ 
```C++
typedef void(* sdsRecPlayEvent_t) (sdsRecPlayId_t id, uint32_t event);
```





**Parameters:**


* `id` handle to SDS Recorder/Player stream 
* `event` event code 




        

<hr>



### typedef sdsRecPlayId\_t 

_handle to SDS Recorder/Player stream_ 
```C++
typedef void* sdsRecPlayId_t;
```




<hr>
## Public Functions Documentation




### function sdsPlayClose 

_Close player stream._ 
```C++
int32_t sdsPlayClose (
    sdsRecPlayId_t id
) 
```





**Parameters:**


* `id` sdsRecPlayId\_t handle to SDS Recorder/Player stream 



**Returns:**

return code 





        

<hr>



### function sdsPlayEndOfStream 

_Check if end of stream has been reached._ 
```C++
int32_t sdsPlayEndOfStream (
    sdsRecPlayId_t id
) 
```





**Parameters:**


* `id` sdsRecPlayId\_t handle to SDS Recorder/Player stream 



**Returns:**

nonzero if end of stream, else 0 





        

<hr>



### function sdsPlayGetSize 

_Get record data size from Player stream._ 
```C++
uint32_t sdsPlayGetSize (
    sdsRecPlayId_t id
) 
```





**Parameters:**


* `id` sdsRecPlayId\_t handle to SDS Recorder/Player stream 



**Returns:**

number of data bytes in record 





        

<hr>



### function sdsPlayRead 

```C++
uint32_t sdsPlayRead (
    sdsRecPlayId_t id,
    uint32_t * timestamp,
    void * buf,
    uint32_t buf_size
) 
```




<hr>



### function sdsRecClose 

_Close recorder stream._ 
```C++
int32_t sdsRecClose (
    sdsRecPlayId_t id
) 
```





**Parameters:**


* `id` sdsRecPlayId\_t handle to SDS Recorder/Player stream 



**Returns:**

return code 





        

<hr>



### function sdsRecPlayInit 

_Initialize recorder and player._ 
```C++
int32_t sdsRecPlayInit (
    sdsRecPlayEvent_t event_cb
) 
```





**Parameters:**


* `event_cb` pointer to sdsRecPlayEvent\_t callback function 



**Returns:**

return code 





        

<hr>



### function sdsRecPlayUninit 

_Uninitialize recorder and player._ 
```C++
int32_t sdsRecPlayUninit (
    void
) 
```





**Returns:**

return code 





        

<hr>



### function sdsRecWrite 

```C++
uint32_t sdsRecWrite (
    sdsRecPlayId_t id,
    uint32_t timestamp,
    const void * buf,
    uint32_t buf_size
) 
```




<hr>
## Macro Definition Documentation





### define SDS\_REC\_PLAY\_ERROR 

_Operation failed._ 
```C++
#define SDS_REC_PLAY_ERROR `(-1)`
```




<hr>



### define SDS\_REC\_PLAY\_EVENT\_IO\_ERROR 

_Event codes._ 
```C++
#define SDS_REC_PLAY_EVENT_IO_ERROR `(1UL << 0)`
```



I/O Error 


        

<hr>



### define SDS\_REC\_PLAY\_OK 

_Function return codes._ 
```C++
#define SDS_REC_PLAY_OK `(0)`
```



Operation completed successfully 


        

<hr>

------------------------------
The documentation for this class was generated from the following file `sds/include/sds_rec_play.h`

