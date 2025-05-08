

# Group SDS\_Recorder\_Player\_Return\_Codes



[**Modules**](modules.md) **>** [**SDS\_Recorder\_Player\_Return\_Codes**](group__SDS__Recorder__Player__Return__Codes.md)



_SDS Recorder and Player Function Return Codes._ [More...](#detailed-description)

































































## Macros

| Type | Name |
| ---: | :--- |
| define  | [**SDS\_PLAY\_EOS**](group__SDS__Recorder__Player__Return__Codes.md#define-sds_play_eos)  `(-7)`<br>_End of stream reached in SDS Player operation._  |
| define  | [**SDS\_PLAY\_ERROR\_NO\_DATA**](group__SDS__Recorder__Player__Return__Codes.md#define-sds_play_error_no_data)  `(-6)`<br>_Not whole data block is available in the SDS Stream buffer._  |
| define  | [**SDS\_REC\_ERROR\_NO\_SPACE**](group__SDS__Recorder__Player__Return__Codes.md#define-sds_rec_error_no_space)  `(-5)`<br>_Insufficient space in SDS Stream buffer to write the entire data block._  |
| define  | [**SDS\_REC\_PLAY\_ERROR**](group__SDS__Recorder__Player__Return__Codes.md#define-sds_rec_play_error)  `(-1)`<br>_General error during SDS Recorder or Player function execution._  |
| define  | [**SDS\_REC\_PLAY\_ERROR\_IO**](group__SDS__Recorder__Player__Return__Codes.md#define-sds_rec_play_error_io)  `(-4)`<br>_I/O error during SDS Recorder or Player function execution._  |
| define  | [**SDS\_REC\_PLAY\_ERROR\_PARAMETER**](group__SDS__Recorder__Player__Return__Codes.md#define-sds_rec_play_error_parameter)  `(-2)`<br>_Invalid parameter passed to an SDS Recorder or Player function._  |
| define  | [**SDS\_REC\_PLAY\_ERROR\_TIMEOUT**](group__SDS__Recorder__Player__Return__Codes.md#define-sds_rec_play_error_timeout)  `(-3)`<br>_SDS Recorder or Player function execution timed out._  |
| define  | [**SDS\_REC\_PLAY\_OK**](group__SDS__Recorder__Player__Return__Codes.md#define-sds_rec_play_ok)  `(0)`<br>_SDS Recorder or Player function executed successfully._  |

## Detailed Description


The following values are returned by most `sdsRec` and `sdsPlay` functions. 


    
## Macro Definition Documentation





### define SDS\_PLAY\_EOS 

_End of stream reached in SDS Player operation._ 
```
#define SDS_PLAY_EOS `(-7)`
```




<hr>



### define SDS\_PLAY\_ERROR\_NO\_DATA 

_Not whole data block is available in the SDS Stream buffer._ 
```
#define SDS_PLAY_ERROR_NO_DATA `(-6)`
```




<hr>



### define SDS\_REC\_ERROR\_NO\_SPACE 

_Insufficient space in SDS Stream buffer to write the entire data block._ 
```
#define SDS_REC_ERROR_NO_SPACE `(-5)`
```




<hr>



### define SDS\_REC\_PLAY\_ERROR 

_General error during SDS Recorder or Player function execution._ 
```
#define SDS_REC_PLAY_ERROR `(-1)`
```




<hr>



### define SDS\_REC\_PLAY\_ERROR\_IO 

_I/O error during SDS Recorder or Player function execution._ 
```
#define SDS_REC_PLAY_ERROR_IO `(-4)`
```




<hr>



### define SDS\_REC\_PLAY\_ERROR\_PARAMETER 

_Invalid parameter passed to an SDS Recorder or Player function._ 
```
#define SDS_REC_PLAY_ERROR_PARAMETER `(-2)`
```




<hr>



### define SDS\_REC\_PLAY\_ERROR\_TIMEOUT 

_SDS Recorder or Player function execution timed out._ 
```
#define SDS_REC_PLAY_ERROR_TIMEOUT `(-3)`
```




<hr>



### define SDS\_REC\_PLAY\_OK 

_SDS Recorder or Player function executed successfully._ 
```
#define SDS_REC_PLAY_OK `(0)`
```




<hr>

------------------------------


