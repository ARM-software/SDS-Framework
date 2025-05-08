

# Group SDS\_Recorder\_Player\_Event\_Codes



[**Modules**](modules.md) **>** [**SDS\_Recorder\_Player\_Event\_Codes**](group__SDS__Recorder__Player__Event__Codes.md)



_SDS Recorder and Player Event Codes._ [More...](#detailed-description)

































































## Macros

| Type | Name |
| ---: | :--- |
| define  | [**SDS\_PLAY\_EVENT\_ERROR\_NO\_DATA**](group__SDS__Recorder__Player__Event__Codes.md#define-sds_play_event_error_no_data)  `(3UL)`<br>_Event triggered when_ [_**sdsPlayRead()**_](group__SDS__Recorder__Player.md#function-sdsplayread) _fails due to insufficient data in the stream buffer._ |
| define  | [**SDS\_REC\_EVENT\_ERROR\_NO\_SPACE**](group__SDS__Recorder__Player__Event__Codes.md#define-sds_rec_event_error_no_space)  `(2UL)`<br>_Event triggered when_ [_**sdsRecWrite()**_](group__SDS__Recorder__Player.md#function-sdsrecwrite) _fails due to insufficient space in the stream buffer._ |
| define  | [**SDS\_REC\_PLAY\_EVENT\_ERROR\_IO**](group__SDS__Recorder__Player__Event__Codes.md#define-sds_rec_play_event_error_io)  `(1UL)`<br>_Event triggered when an I/O error occurs during recording or playback._  |

## Detailed Description


The following values are passed as event value to [**sdsRecPlayEvent\_t**](group__SDS__Recorder__Player.md#typedef-sdsrecplayevent_t) callback function. 


    
## Macro Definition Documentation





### define SDS\_PLAY\_EVENT\_ERROR\_NO\_DATA 

_Event triggered when_ [_**sdsPlayRead()**_](group__SDS__Recorder__Player.md#function-sdsplayread) _fails due to insufficient data in the stream buffer._
```
#define SDS_PLAY_EVENT_ERROR_NO_DATA `(3UL)`
```




<hr>



### define SDS\_REC\_EVENT\_ERROR\_NO\_SPACE 

_Event triggered when_ [_**sdsRecWrite()**_](group__SDS__Recorder__Player.md#function-sdsrecwrite) _fails due to insufficient space in the stream buffer._
```
#define SDS_REC_EVENT_ERROR_NO_SPACE `(2UL)`
```




<hr>



### define SDS\_REC\_PLAY\_EVENT\_ERROR\_IO 

_Event triggered when an I/O error occurs during recording or playback._ 
```
#define SDS_REC_PLAY_EVENT_ERROR_IO `(1UL)`
```




<hr>

------------------------------


