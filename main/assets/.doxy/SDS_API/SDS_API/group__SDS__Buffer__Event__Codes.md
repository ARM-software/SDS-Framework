

# Group SDS\_Buffer\_Event\_Codes



[**Modules**](modules.md) **>** [**SDS\_Buffer\_Event\_Codes**](group__SDS__Buffer__Event__Codes.md)



_SDS Buffer Event Codes._ [More...](#detailed-description)

































































## Macros

| Type | Name |
| ---: | :--- |
| define  | [**SDS\_BUFFER\_EVENT\_DATA\_HIGH**](group__SDS__Buffer__Event__Codes.md#define-sds_buffer_event_data_high)  `(2UL)`<br>_Event signaled when data in circular buffer exceeds high threshold value._  |
| define  | [**SDS\_BUFFER\_EVENT\_DATA\_LOW**](group__SDS__Buffer__Event__Codes.md#define-sds_buffer_event_data_low)  `(1UL)`<br>_Event signaled when data in circular buffer drops below low threshold value._  |

## Detailed Description


The following values are passed as event value to [**sdsBufferEvent\_t**](group__SDS__Buffer.md#typedef-sdsbufferevent_t) callback function. 


    
## Macro Definition Documentation





### define SDS\_BUFFER\_EVENT\_DATA\_HIGH 

_Event signaled when data in circular buffer exceeds high threshold value._ 
```
#define SDS_BUFFER_EVENT_DATA_HIGH `(2UL)`
```




<hr>



### define SDS\_BUFFER\_EVENT\_DATA\_LOW 

_Event signaled when data in circular buffer drops below low threshold value._ 
```
#define SDS_BUFFER_EVENT_DATA_LOW `(1UL)`
```




<hr>

------------------------------


