

# Group SDS\_Event\_Codes



[**Modules**](modules.md) **>** [**SDS\_Event\_Codes**](group__SDS__Event__Codes.md)



_SDS Event Codes._ [More...](#detailed-description)

































































## Macros

| Type | Name |
| ---: | :--- |
| define  | [**SDS\_EVENT\_ERROR\_IO**](group__SDS__Event__Codes.md#define-sds_event_error_io)  `(1UL)`<br>_Event triggered when an SDS I/O error occurs._  |
| define  | [**SDS\_EVENT\_NO\_DATA**](group__SDS__Event__Codes.md#define-sds_event_no_data)  `(4UL)`<br>_Event triggered when_ [_**sdsRead**_](group__SDS__Stream__Interface.md#function-sdsread) _fails due to insufficient data in the SDS circular buffer._ |
| define  | [**SDS\_EVENT\_NO\_SPACE**](group__SDS__Event__Codes.md#define-sds_event_no_space)  `(2UL)`<br>_Event triggered when_ [_**sdsWrite**_](group__SDS__Stream__Interface.md#function-sdswrite) _fails due to insufficient space in the SDS circular buffer._ |

## Detailed Description


The following values are passed as event value to the [**sdsEvent\_t**](group__SDS__Stream__Interface.md#typedef-sdsevent_t) callback function. 


    
## Macro Definition Documentation





### define SDS\_EVENT\_ERROR\_IO 

_Event triggered when an SDS I/O error occurs._ 
```
#define SDS_EVENT_ERROR_IO `(1UL)`
```




<hr>



### define SDS\_EVENT\_NO\_DATA 

_Event triggered when_ [_**sdsRead**_](group__SDS__Stream__Interface.md#function-sdsread) _fails due to insufficient data in the SDS circular buffer._
```
#define SDS_EVENT_NO_DATA `(4UL)`
```




<hr>



### define SDS\_EVENT\_NO\_SPACE 

_Event triggered when_ [_**sdsWrite**_](group__SDS__Stream__Interface.md#function-sdswrite) _fails due to insufficient space in the SDS circular buffer._
```
#define SDS_EVENT_NO_SPACE `(2UL)`
```




<hr>

------------------------------


