

# File sds\_rec.h



[**FileList**](files.md) **>** [**include**](dir_d09908635ef304ba819d3349bcb716bf.md) **>** [**sds\_rec.h**](sds__rec_8h.md)

[Go to the source code of this file](sds__rec_8h_source.md)



* `#include <stdint.h>`

















## Public Types

| Type | Name |
| ---: | :--- |
| typedef void(\* | [**sdsRecEvent\_t**](#typedef-sdsrecevent_t)  <br>_Event callback function._  |
| typedef void \* | [**sdsRecId\_t**](#typedef-sdsrecid_t)  <br>_Identifier._  |















































## Macros

| Type | Name |
| ---: | :--- |
| define  | [**SDS\_REC\_ERROR**](sds__rec_8h.md#define-sds_rec_error)  `(-1)`<br>_Operation failed._  |
| define  | [**SDS\_REC\_EVENT\_IO\_ERROR**](sds__rec_8h.md#define-sds_rec_event_io_error)  `(1UL &lt;&lt; 0)`<br>_Events._  |
| define  | [**SDS\_REC\_OK**](sds__rec_8h.md#define-sds_rec_ok)  `(0)`<br>_Function return codes._  |

## Public Types Documentation




### typedef sdsRecEvent\_t 

_Event callback function._ 
```C++
typedef void(* sdsRecEvent_t) (sdsRecId_t id, uint32_t event);
```




<hr>



### typedef sdsRecId\_t 

_Identifier._ 
```C++
typedef void* sdsRecId_t;
```



handle to SDS file for recording 


        

<hr>
## Macro Definition Documentation





### define SDS\_REC\_ERROR 

_Operation failed._ 
```C++
#define SDS_REC_ERROR `(-1)`
```




<hr>



### define SDS\_REC\_EVENT\_IO\_ERROR 

_Events._ 
```C++
#define SDS_REC_EVENT_IO_ERROR `(1UL << 0)`
```



I/O Error 


        

<hr>



### define SDS\_REC\_OK 

_Function return codes._ 
```C++
#define SDS_REC_OK `(0)`
```



Operation completed successfully 


        

<hr>

------------------------------
The documentation for this class was generated from the following file `sds/include/sds_rec.h`

