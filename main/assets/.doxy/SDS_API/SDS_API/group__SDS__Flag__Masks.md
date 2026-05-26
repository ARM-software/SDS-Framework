

# Group SDS\_Flag\_Masks



[**Modules**](modules.md) **>** [**SDS\_Flag\_Masks**](group__SDS__Flag__Masks.md)



_SDS Flag Bitmasks._ [More...](#detailed-description)

































































## Macros

| Type | Name |
| ---: | :--- |
| define  | [**SDS\_FLAG\_ALIVE**](group__SDS__Flag__Masks.md#define-sds_flag_alive)  `(0x10000000UL)`<br>_Flag used by host to signal it is alive (set when the host is alive; unset when the host is not alive)._  |
| define  | [**SDS\_FLAG\_PLAYBACK**](group__SDS__Flag__Masks.md#define-sds_flag_playback)  `(0x20000000UL)`<br>_Flag for switching between recording and playback mode (set for playback; unset for recording)._  |
| define  | [**SDS\_FLAG\_RESET**](group__SDS__Flag__Masks.md#define-sds_flag_reset)  `(0x08000000UL)`<br>_Flag used to request firmware reset (set to reset the firmware)._  |
| define  | [**SDS\_FLAG\_START**](group__SDS__Flag__Masks.md#define-sds_flag_start)  `(0x80000000UL)`<br>_Flag controlling streaming (set to start, cleared to stop)._  |
| define  | [**SDS\_FLAG\_TERMINATE**](group__SDS__Flag__Masks.md#define-sds_flag_terminate)  `(0x40000000UL)`<br>_Flag for terminating CI run (on FVP simulation or pyOCD)._  |

## Detailed Description


The following values are used by function [**sdsFlagsModify**](group__SDS__Stream__Interface.md#function-sdsflagsmodify) and global variable [**sdsFlags**](group__SDS__Stream__Interface.md#variable-sdsflags). 


    
## Macro Definition Documentation





### define SDS\_FLAG\_ALIVE 

_Flag used by host to signal it is alive (set when the host is alive; unset when the host is not alive)._ 
```
#define SDS_FLAG_ALIVE `(0x10000000UL)`
```




<hr>



### define SDS\_FLAG\_PLAYBACK 

_Flag for switching between recording and playback mode (set for playback; unset for recording)._ 
```
#define SDS_FLAG_PLAYBACK `(0x20000000UL)`
```




<hr>



### define SDS\_FLAG\_RESET 

_Flag used to request firmware reset (set to reset the firmware)._ 
```
#define SDS_FLAG_RESET `(0x08000000UL)`
```




<hr>



### define SDS\_FLAG\_START 

_Flag controlling streaming (set to start, cleared to stop)._ 
```
#define SDS_FLAG_START `(0x80000000UL)`
```




<hr>



### define SDS\_FLAG\_TERMINATE 

_Flag for terminating CI run (on FVP simulation or pyOCD)._ 
```
#define SDS_FLAG_TERMINATE `(0x40000000UL)`
```




<hr>

------------------------------


