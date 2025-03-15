

# File sds.h



[**FileList**](files.md) **>** [**include**](dir_d09908635ef304ba819d3349bcb716bf.md) **>** [**sds.h**](sds_8h.md)

[Go to the source code of this file](sds_8h_source.md)



* `#include <stdint.h>`

















## Public Types

| Type | Name |
| ---: | :--- |
| typedef void(\* | [**sdsEvent\_t**](#typedef-sdsevent_t)  <br> |




















## Public Functions

| Type | Name |
| ---: | :--- |
|  [**sdsId\_t**](group__SDS__Circular__Buffer.md#typedef-sdsid_t) | [**sdsOpen**](#function-sdsopen) (void \* buf, uint32\_t buf\_size, uint32\_t threshold\_low, uint32\_t threshold\_high) <br> |



























## Macros

| Type | Name |
| ---: | :--- |
| define  | [**SDS\_ERROR**](sds_8h.md#define-sds_error)  `(-1)`<br>_Operation failed._  |
| define  | [**SDS\_EVENT\_DATA\_HIGH**](sds_8h.md#define-sds_event_data_high)  `(1UL &lt;&lt; 1)`<br>_Data above or equal to threshold._  |
| define  | [**SDS\_EVENT\_DATA\_LOW**](sds_8h.md#define-sds_event_data_low)  `(1UL &lt;&lt; 0)`<br>_Events._  |
| define  | [**SDS\_OK**](sds_8h.md#define-sds_ok)  `(0)`<br>_Function return codes._  |

## Public Types Documentation




### typedef sdsEvent\_t 

```C++
typedef void(* sdsEvent_t) (sdsId_t id, uint32_t event, void *arg);
```




<hr>
## Public Functions Documentation




### function sdsOpen 

```C++
sdsId_t sdsOpen (
    void * buf,
    uint32_t buf_size,
    uint32_t threshold_low,
    uint32_t threshold_high
) 
```




<hr>
## Macro Definition Documentation





### define SDS\_ERROR 

_Operation failed._ 
```C++
#define SDS_ERROR `(-1)`
```




<hr>



### define SDS\_EVENT\_DATA\_HIGH 

_Data above or equal to threshold._ 
```C++
#define SDS_EVENT_DATA_HIGH `(1UL << 1)`
```




<hr>



### define SDS\_EVENT\_DATA\_LOW 

_Events._ 
```C++
#define SDS_EVENT_DATA_LOW `(1UL << 0)`
```



Data bellow or equal to threshold 


        

<hr>



### define SDS\_OK 

_Function return codes._ 
```C++
#define SDS_OK `(0)`
```



Operation completed successfully 


        

<hr>

------------------------------
The documentation for this class was generated from the following file `sds/include/sds.h`

