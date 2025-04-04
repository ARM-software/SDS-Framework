

# File sds\_buffer.h



[**FileList**](files.md) **>** [**include**](dir_d09908635ef304ba819d3349bcb716bf.md) **>** [**sds\_buffer.h**](sds__buffer_8h.md)

[Go to the source code of this file](sds__buffer_8h_source.md)



* `#include <stdint.h>`
































































## Macros

| Type | Name |
| ---: | :--- |
| define  | [**SDS\_BUFFER\_ERROR**](sds__buffer_8h.md#define-sds_buffer_error)  `(-1)`<br>_Operation failed._  |
| define  | [**SDS\_BUFFER\_EVENT\_DATA\_HIGH**](sds__buffer_8h.md#define-sds_buffer_event_data_high)  `(1UL &lt;&lt; 1)`<br>_Data above or equal to threshold._  |
| define  | [**SDS\_BUFFER\_EVENT\_DATA\_LOW**](sds__buffer_8h.md#define-sds_buffer_event_data_low)  `(1UL &lt;&lt; 0)`<br>_Events._  |
| define  | [**SDS\_BUFFER\_OK**](sds__buffer_8h.md#define-sds_buffer_ok)  `(0)`<br>_Function return codes._  |

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

