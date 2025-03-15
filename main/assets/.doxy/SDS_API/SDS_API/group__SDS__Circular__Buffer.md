

# Group SDS\_Circular\_Buffer



[**Modules**](modules.md) **>** [**SDS\_Circular\_Buffer**](group__SDS__Circular__Buffer.md)



sds.h _: SDS circular buffer handling for data streams_[More...](#detailed-description)


















## Public Types

| Type | Name |
| ---: | :--- |
| typedef void \* | [**sdsId\_t**](#typedef-sdsid_t)  <br>_Identifier._  |
| typedef void(\* | [**sdsPlayEvent\_t**](#typedef-sdsplayevent_t)  <br>_Call back function for SDS circular buffer handling._  |




















## Public Functions

| Type | Name |
| ---: | :--- |
|  int32\_t | [**sdsClear**](#function-sdsclear) ([**sdsId\_t**](group__SDS__Circular__Buffer.md#typedef-sdsid_t) id) <br>_Clear stream data._  |
|  int32\_t | [**sdsClose**](#function-sdsclose) ([**sdsId\_t**](group__SDS__Circular__Buffer.md#typedef-sdsid_t) id) <br>_Close stream._  |
|  uint32\_t | [**sdsGetCount**](#function-sdsgetcount) ([**sdsId\_t**](group__SDS__Circular__Buffer.md#typedef-sdsid_t) id) <br>_Get data count in stream._  |
|  uint32\_t | [**sdsRead**](#function-sdsread) ([**sdsId\_t**](group__SDS__Circular__Buffer.md#typedef-sdsid_t) id, void \* buf, uint32\_t buf\_size) <br>_Read data from stream._  |
|  int32\_t | [**sdsRegisterEvents**](#function-sdsregisterevents) ([**sdsId\_t**](group__SDS__Circular__Buffer.md#typedef-sdsid_t) id, sdsEvent\_t event\_cb, uint32\_t event\_mask, void \* event\_arg) <br>_Register stream events._  |
|  uint32\_t | [**sdsWrite**](#function-sdswrite) ([**sdsId\_t**](group__SDS__Circular__Buffer.md#typedef-sdsid_t) id, const void \* buf, uint32\_t buf\_size) <br>_Write data to stream._  |




























## Detailed Description


/


/\*=======0=========1=========2=========3=========4=========5=========6=========7=========8=========9=========0=========1====


The stream data is written to and read from a circular buffer allocated in RAM. Event callback function which is triggered on data threshold reached can be registered. 


    
## Public Types Documentation




### typedef sdsId\_t 

_Identifier._ 
```
typedef void* sdsId_t;
```



handle to SDS stream


todo test 


        

<hr>



### typedef sdsPlayEvent\_t 

_Call back function for SDS circular buffer handling._ 
```
typedef void(* sdsPlayEvent_t) (sdsPlayId_t id, uint32_t event);
```



Call back function for player events.




**Parameters:**


* `id` handle to SDS file for playback 
* `event` event code

todo 


        

<hr>
## Public Functions Documentation




### function sdsClear 

_Clear stream data._ 
```
int32_t sdsClear (
    sdsId_t id
) 
```





**Parameters:**


* `id` [**sdsId\_t**](group__SDS__Circular__Buffer.md#typedef-sdsid_t) handle to SDS stream 



**Returns:**

return code


todo 


        

<hr>



### function sdsClose 

_Close stream._ 
```
int32_t sdsClose (
    sdsId_t id
) 
```





**Parameters:**


* `id` [**sdsId\_t**](group__SDS__Circular__Buffer.md#typedef-sdsid_t) handle to SDS stream 



**Returns:**

return code


todo 


        

<hr>



### function sdsGetCount 

_Get data count in stream._ 
```
uint32_t sdsGetCount (
    sdsId_t id
) 
```





**Parameters:**


* `id` [**sdsId\_t**](group__SDS__Circular__Buffer.md#typedef-sdsid_t) handle to SDS stream 



**Returns:**

number of bytes in stream


todo 


        

<hr>



### function sdsRead 

_Read data from stream._ 
```
uint32_t sdsRead (
    sdsId_t id,
    void * buf,
    uint32_t buf_size
) 
```





**Parameters:**


* `id` [**sdsId\_t**](group__SDS__Circular__Buffer.md#typedef-sdsid_t) handle to SDS stream 
* `buf` pointer to buffer for data to read 
* `buf_size` buffer size in bytes 



**Returns:**

number of bytes read


todo 


        

<hr>



### function sdsRegisterEvents 

_Register stream events._ 
```
int32_t sdsRegisterEvents (
    sdsId_t id,
    sdsEvent_t event_cb,
    uint32_t event_mask,
    void * event_arg
) 
```





**Parameters:**


* `id` [**sdsId\_t**](group__SDS__Circular__Buffer.md#typedef-sdsid_t) handle to SDS stream 
* `event_cb` pointer to sdsEvent\_t 
* `event_mask` event mask 
* `event_arg` event argument 



**Returns:**

return code


todo 


        

<hr>



### function sdsWrite 

_Write data to stream._ 
```
uint32_t sdsWrite (
    sdsId_t id,
    const void * buf,
    uint32_t buf_size
) 
```





**Parameters:**


* `id` [**sdsId\_t**](group__SDS__Circular__Buffer.md#typedef-sdsid_t) handle to SDS stream 
* `buf` pointer to buffer with data to write 
* `buf_size` buffer size in bytes 



**Returns:**

number of bytes written


todo 


        

<hr>

------------------------------


