

# File sds\_rec\_play.h



[**FileList**](files.md) **>** [**include**](dir_d09908635ef304ba819d3349bcb716bf.md) **>** [**sds\_rec\_play.h**](sds__rec__play_8h.md)

[Go to the source code of this file](sds__rec__play_8h_source.md)



* `#include <stdint.h>`

















## Public Types

| Type | Name |
| ---: | :--- |
| typedef void(\* | [**sdsRecPlayEvent\_t**](#typedef-sdsrecplayevent_t)  <br>_Callback function for recorder and player events._  |















































## Macros

| Type | Name |
| ---: | :--- |
| define  | [**SDS\_REC\_PLAY\_ERROR**](sds__rec__play_8h.md#define-sds_rec_play_error)  `(-1)`<br>_Operation failed._  |
| define  | [**SDS\_REC\_PLAY\_ERROR\_TIMEOUT**](sds__rec__play_8h.md#define-sds_rec_play_error_timeout)  `(-2)`<br>_Operation failed: Timeout._  |
| define  | [**SDS\_REC\_PLAY\_EVENT\_IO\_ERROR**](sds__rec__play_8h.md#define-sds_rec_play_event_io_error)  `(1UL &lt;&lt; 0)`<br>_Event codes._  |
| define  | [**SDS\_REC\_PLAY\_OK**](sds__rec__play_8h.md#define-sds_rec_play_ok)  `(0)`<br>_Function return codes._  |

## Public Types Documentation




### typedef sdsRecPlayEvent\_t 

_Callback function for recorder and player events._ 
```C++
typedef void(* sdsRecPlayEvent_t) (sdsRecPlayId_t id, uint32_t event);
```





**Parameters:**


* `id` handle to SDS Recorder/Player stream 
* `event` event code 




        

<hr>
## Macro Definition Documentation





### define SDS\_REC\_PLAY\_ERROR 

_Operation failed._ 
```C++
#define SDS_REC_PLAY_ERROR `(-1)`
```




<hr>



### define SDS\_REC\_PLAY\_ERROR\_TIMEOUT 

_Operation failed: Timeout._ 
```C++
#define SDS_REC_PLAY_ERROR_TIMEOUT `(-2)`
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

