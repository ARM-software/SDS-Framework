

# Group SDS\_State\_Codes



[**Modules**](modules.md) **>** [**SDS\_State\_Codes**](group__SDS__State__Codes.md)



_SDS State Codes._ [More...](#detailed-description)

































































## Macros

| Type | Name |
| ---: | :--- |
| define  | [**SDS\_STATE\_ACTIVE**](group__SDS__State__Codes.md#define-sds_state_active)  `(3UL)`<br>_Streaming is active._  |
| define  | [**SDS\_STATE\_CONNECTED**](group__SDS__State__Codes.md#define-sds_state_connected)  `(1UL)`<br>_Device is connected to the host, but streaming is not active._  |
| define  | [**SDS\_STATE\_END**](group__SDS__State__Codes.md#define-sds_state_end)  `(6UL)`<br>_Request to end streaming (e.g., no more playback data is available)._  |
| define  | [**SDS\_STATE\_INACTIVE**](group__SDS__State__Codes.md#define-sds_state_inactive)  `(0UL)`<br>_Device is not connected to the host and streaming is not active._  |
| define  | [**SDS\_STATE\_RESET**](group__SDS__State__Codes.md#define-sds_state_reset)  `(7UL)`<br>_Request to reset the device._  |
| define  | [**SDS\_STATE\_START**](group__SDS__State__Codes.md#define-sds_state_start)  `(2UL)`<br>_Request to start streaming; open streams and get ready for read/write operations._  |
| define  | [**SDS\_STATE\_STOP\_DONE**](group__SDS__State__Codes.md#define-sds_state_stop_done)  `(5UL)`<br>_Streaming has stopped._  |
| define  | [**SDS\_STATE\_STOP\_REQ**](group__SDS__State__Codes.md#define-sds_state_stop_req)  `(4UL)`<br>_Request to stop streaming and close all open streams._  |
| define  | [**SDS\_STATE\_TERMINATE**](group__SDS__State__Codes.md#define-sds_state_terminate)  `(8UL)`<br>_Request to terminate the active session._  |

## Detailed Description


The following values are used by global variable [**sdsState**](group__SDS__Interface.md#variable-sdsstate). 


    
## Macro Definition Documentation





### define SDS\_STATE\_ACTIVE 

_Streaming is active._ 
```
#define SDS_STATE_ACTIVE `(3UL)`
```




<hr>



### define SDS\_STATE\_CONNECTED 

_Device is connected to the host, but streaming is not active._ 
```
#define SDS_STATE_CONNECTED `(1UL)`
```




<hr>



### define SDS\_STATE\_END 

_Request to end streaming (e.g., no more playback data is available)._ 
```
#define SDS_STATE_END `(6UL)`
```




<hr>



### define SDS\_STATE\_INACTIVE 

_Device is not connected to the host and streaming is not active._ 
```
#define SDS_STATE_INACTIVE `(0UL)`
```




<hr>



### define SDS\_STATE\_RESET 

_Request to reset the device._ 
```
#define SDS_STATE_RESET `(7UL)`
```




<hr>



### define SDS\_STATE\_START 

_Request to start streaming; open streams and get ready for read/write operations._ 
```
#define SDS_STATE_START `(2UL)`
```




<hr>



### define SDS\_STATE\_STOP\_DONE 

_Streaming has stopped._ 
```
#define SDS_STATE_STOP_DONE `(5UL)`
```




<hr>



### define SDS\_STATE\_STOP\_REQ 

_Request to stop streaming and close all open streams._ 
```
#define SDS_STATE_STOP_REQ `(4UL)`
```




<hr>



### define SDS\_STATE\_TERMINATE 

_Request to terminate the active session._ 
```
#define SDS_STATE_TERMINATE `(8UL)`
```




<hr>

------------------------------


