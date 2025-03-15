

# Group SDS\_Recorder



[**Modules**](modules.md) **>** [**SDS\_Recorder**](group__SDS__Recorder.md)



sds\_rec.h _: SDS Recorder for writing SDS files via communication or file I/O interface._[More...](#detailed-description)






































## Public Functions

| Type | Name |
| ---: | :--- |
|  int32\_t | [**sdsRecClose**](#function-sdsrecclose) (sdsRecId\_t id) <br>_Close recorder stream._  |
|  int32\_t | [**sdsRecInit**](#function-sdsrecinit) (sdsRecEvent\_t event\_cb) <br>_Initialize recorder._  |
|  sdsRecId\_t | [**sdsRecOpen**](#function-sdsrecopen) (const char \* name, void \* buf, uint32\_t buf\_size, uint32\_t io\_threshold) <br>_Open recorder stream._  |
|  int32\_t | [**sdsRecUninit**](#function-sdsrecuninit) (void) <br>_Uninitialize recorder._  |
|  uint32\_t | [**sdsRecWrite**](#function-sdsrecwrite) (sdsRecId\_t id, uint32\_t timestamp, const void \* buf, uint32\_t buf\_size) <br> |




























## Detailed Description


Todo 


    
## Public Functions Documentation




### function sdsRecClose 

_Close recorder stream._ 
```
int32_t sdsRecClose (
    sdsRecId_t id
) 
```



todo




**Parameters:**


* `id` sdsRecId\_t handle to SDS file for recording 



**Returns:**

return code 





        

<hr>



### function sdsRecInit 

_Initialize recorder._ 
```
int32_t sdsRecInit (
    sdsRecEvent_t event_cb
) 
```



todo




**Parameters:**


* `event_cb` pointer to sdsRecEvent\_t callback function 



**Returns:**

return code 





        

<hr>



### function sdsRecOpen 

_Open recorder stream._ 
```
sdsRecId_t sdsRecOpen (
    const char * name,
    void * buf,
    uint32_t buf_size,
    uint32_t io_threshold
) 
```



todo




**Parameters:**


* `name` stream name (pointer to NULL terminated string) 
* `buf` pointer to buffer for stream 
* `buf_size` buffer size in bytes 
* `io_threshold` threshold in bytes to trigger I/O write (when equal or above threshold) 



**Returns:**

sdsRecId\_t handle to SDS file for recording 





        

<hr>



### function sdsRecUninit 

_Uninitialize recorder._ 
```
int32_t sdsRecUninit (
    void
) 
```



todo




**Returns:**

return code 





        

<hr>



### function sdsRecWrite 

```
uint32_t sdsRecWrite (
    sdsRecId_t id,
    uint32_t timestamp,
    const void * buf,
    uint32_t buf_size
) 
```



todo 


        

<hr>

------------------------------


