

# Group SDS\_Player



[**Modules**](modules.md) **>** [**SDS\_Player**](group__SDS__Player.md)



sds\_play.h _: SDS Player for reading SDS files via communication or file I/O interface._[More...](#detailed-description)


















## Public Types

| Type | Name |
| ---: | :--- |
| typedef void \* | [**sdsPlayId\_t**](#typedef-sdsplayid_t)  <br>_Handle to SDS file for playback._  |




















## Public Functions

| Type | Name |
| ---: | :--- |
|  int32\_t | [**sdsPlayClose**](#function-sdsplayclose) ([**sdsPlayId\_t**](group__SDS__Player.md#typedef-sdsplayid_t) id) <br>_Close player stream._  |
|  int32\_t | [**sdsPlayEndOfStream**](#function-sdsplayendofstream) ([**sdsPlayId\_t**](group__SDS__Player.md#typedef-sdsplayid_t) id) <br>_Check if end of stream has been reached._  |
|  uint32\_t | [**sdsPlayGetSize**](#function-sdsplaygetsize) ([**sdsPlayId\_t**](group__SDS__Player.md#typedef-sdsplayid_t) id) <br>_Get record data size from Player stream._  |
|  [**sdsPlayId\_t**](group__SDS__Player.md#typedef-sdsplayid_t) | [**sdsPlayOpen**](#function-sdsplayopen) (const char \* name, void \* buf, uint32\_t buf\_size, uint32\_t io\_threshold) <br>_Open player stream._  |
|  uint32\_t | [**sdsPlayRead**](#function-sdsplayread) ([**sdsPlayId\_t**](group__SDS__Player.md#typedef-sdsplayid_t) id, uint32\_t \* timestamp, void \* buf, uint32\_t buf\_size) <br> |
|  int32\_t | [**sdsPlayUninit**](#function-sdsplayuninit) (void) <br>_Uninitialize player._  |




























## Detailed Description


The SDS Player functions allow to playback SDS files via a file I/O interface. The files are either read via a communication stack that is connected to the SDSIO server or via a File System interface that uses a memory card in the target system as storage. 


    
## Public Types Documentation




### typedef sdsPlayId\_t 

_Handle to SDS file for playback._ 
```
typedef void* sdsPlayId_t;
```



todo 


        

<hr>
## Public Functions Documentation




### function sdsPlayClose 

_Close player stream._ 
```
int32_t sdsPlayClose (
    sdsPlayId_t id
) 
```



todo x




**Parameters:**


* `id` [**sdsPlayId\_t**](group__SDS__Player.md#typedef-sdsplayid_t) handle to SDS file for playback 



**Returns:**

return code 





        

<hr>



### function sdsPlayEndOfStream 

_Check if end of stream has been reached._ 
```
int32_t sdsPlayEndOfStream (
    sdsPlayId_t id
) 
```



todo




**Parameters:**


* `id` [**sdsPlayId\_t**](group__SDS__Player.md#typedef-sdsplayid_t) handle to SDS file for playback 



**Returns:**

nonzero if end of stream, else 0 





        

<hr>



### function sdsPlayGetSize 

_Get record data size from Player stream._ 
```
uint32_t sdsPlayGetSize (
    sdsPlayId_t id
) 
```



todo




**Parameters:**


* `id` [**sdsPlayId\_t**](group__SDS__Player.md#typedef-sdsplayid_t) handle to SDS file for playback 



**Returns:**

number of data bytes in record 





        

<hr>



### function sdsPlayOpen 

_Open player stream._ 
```
sdsPlayId_t sdsPlayOpen (
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

[**sdsPlayId\_t**](group__SDS__Player.md#typedef-sdsplayid_t) handle to SDS file for playback 





        

<hr>



### function sdsPlayRead 

```
uint32_t sdsPlayRead (
    sdsPlayId_t id,
    uint32_t * timestamp,
    void * buf,
    uint32_t buf_size
) 
```



todo x 


        

<hr>



### function sdsPlayUninit 

_Uninitialize player._ 
```
int32_t sdsPlayUninit (
    void
) 
```



todo




**Returns:**

return code 





        

<hr>

------------------------------


