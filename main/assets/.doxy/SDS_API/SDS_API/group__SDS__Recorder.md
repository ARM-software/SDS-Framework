

# Group SDS\_Recorder



[**Modules**](modules.md) **>** [**SDS\_Recorder**](group__SDS__Recorder.md)



_sds\_rec.h: SDS Recorder for writing SDS files via communication or file I/O interface._ [More...](#detailed-description)






































## Public Functions

| Type | Name |
| ---: | :--- |
|  sdsRecPlayId\_t | [**sdsRecOpen**](#function-sdsrecopen) (const char \* name, void \* buf, uint32\_t buf\_size, uint32\_t io\_threshold) <br>_Open recorder stream._  |




























## Detailed Description


Todo 


    
## Public Functions Documentation




### function sdsRecOpen 

_Open recorder stream._ 
```
sdsRecPlayId_t sdsRecOpen (
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

sdsRecPlayId\_t handle to SDS Recorder/Player stream 





        

<hr>

------------------------------


