

# Group SDS\_Player



[**Modules**](modules.md) **>** [**SDS\_Player**](group__SDS__Player.md)



_sds\_play.h: SDS Player for reading SDS files via communication or file I/O interface._ [More...](#detailed-description)






































## Public Functions

| Type | Name |
| ---: | :--- |
|  sdsRecPlayId\_t | [**sdsPlayOpen**](#function-sdsplayopen) (const char \* name, void \* buf, uint32\_t buf\_size, uint32\_t io\_threshold) <br>_Open player stream._  |




























## Detailed Description


The SDS Player functions allow to playback SDS files via a file I/O interface. The files are either read via a communication stack that is connected to the SDSIO server or via a File System interface that uses a memory card in the target system as storage. 


    
## Public Functions Documentation




### function sdsPlayOpen 

_Open player stream._ 
```
sdsRecPlayId_t sdsPlayOpen (
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
* `io_threshold` threshold in bytes to trigger I/O read (when below threshold) 



**Returns:**

sdsRecPlayId\_t handle to SDS Recorder/Player stream 





        

<hr>

------------------------------


