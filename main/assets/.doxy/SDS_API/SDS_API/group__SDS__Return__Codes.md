

# Group SDS\_Return\_Codes



[**Modules**](modules.md) **>** [**SDS\_Return\_Codes**](group__SDS__Return__Codes.md)



_SDS Function Return Codes._ [More...](#detailed-description)

































































## Macros

| Type | Name |
| ---: | :--- |
| define  | [**SDS\_EOS**](group__SDS__Return__Codes.md#define-sds_eos)  `(-7)`<br>_End of stream reached in read operation._  |
| define  | [**SDS\_ERROR**](group__SDS__Return__Codes.md#define-sds_error)  `(-1)`<br>_Unspecified error._  |
| define  | [**SDS\_ERROR\_IO**](group__SDS__Return__Codes.md#define-sds_error_io)  `(-4)`<br>_I/O error during function execution._  |
| define  | [**SDS\_ERROR\_PARAMETER**](group__SDS__Return__Codes.md#define-sds_error_parameter)  `(-2)`<br>_Invalid parameter passed to a function._  |
| define  | [**SDS\_ERROR\_TIMEOUT**](group__SDS__Return__Codes.md#define-sds_error_timeout)  `(-3)`<br>_Function execution timed out._  |
| define  | [**SDS\_NO\_DATA**](group__SDS__Return__Codes.md#define-sds_no_data)  `(-6)`<br>_Insufficient data in SDS stream buffer._  |
| define  | [**SDS\_NO\_SPACE**](group__SDS__Return__Codes.md#define-sds_no_space)  `(-5)`<br>_Insufficient space in SDS circular buffer to write the entire data block._  |
| define  | [**SDS\_OK**](group__SDS__Return__Codes.md#define-sds_ok)  `(0)`<br>_Function completed successfully._  |

## Detailed Description


The following values are returned by most SDS functions. 


    
## Macro Definition Documentation





### define SDS\_EOS 

_End of stream reached in read operation._ 
```
#define SDS_EOS `(-7)`
```




<hr>



### define SDS\_ERROR 

_Unspecified error._ 
```
#define SDS_ERROR `(-1)`
```




<hr>



### define SDS\_ERROR\_IO 

_I/O error during function execution._ 
```
#define SDS_ERROR_IO `(-4)`
```




<hr>



### define SDS\_ERROR\_PARAMETER 

_Invalid parameter passed to a function._ 
```
#define SDS_ERROR_PARAMETER `(-2)`
```




<hr>



### define SDS\_ERROR\_TIMEOUT 

_Function execution timed out._ 
```
#define SDS_ERROR_TIMEOUT `(-3)`
```




<hr>



### define SDS\_NO\_DATA 

_Insufficient data in SDS stream buffer._ 
```
#define SDS_NO_DATA `(-6)`
```




<hr>



### define SDS\_NO\_SPACE 

_Insufficient space in SDS circular buffer to write the entire data block._ 
```
#define SDS_NO_SPACE `(-5)`
```




<hr>



### define SDS\_OK 

_Function completed successfully._ 
```
#define SDS_OK `(0)`
```




<hr>

------------------------------


