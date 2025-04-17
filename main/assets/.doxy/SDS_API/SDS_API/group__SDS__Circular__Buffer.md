

# Group SDS\_Circular\_Buffer



[**Modules**](modules.md) **>** [**SDS\_Circular\_Buffer**](group__SDS__Circular__Buffer.md)



_sds.h: SDS circular buffer handling for data streams_ [More...](#detailed-description)


















## Public Types

| Type | Name |
| ---: | :--- |
| typedef void(\* | [**sdsBufferEvent\_t**](#typedef-sdsbufferevent_t)  <br>_Callback function for SDS circular buffer handling._  |
| typedef void \* | [**sdsBufferId\_t**](#typedef-sdsbufferid_t)  <br>_Handle to SDS buffer stream._  |




















## Public Functions

| Type | Name |
| ---: | :--- |
|  int32\_t | [**sdsBufferClear**](#function-sdsbufferclear) ([**sdsBufferId\_t**](group__SDS__Circular__Buffer.md#typedef-sdsbufferid_t) id) <br>_Clear SDS buffer stream data._  |
|  int32\_t | [**sdsBufferClose**](#function-sdsbufferclose) ([**sdsBufferId\_t**](group__SDS__Circular__Buffer.md#typedef-sdsbufferid_t) id) <br>_Close SDS buffer stream._  |
|  uint32\_t | [**sdsBufferGetCount**](#function-sdsbuffergetcount) ([**sdsBufferId\_t**](group__SDS__Circular__Buffer.md#typedef-sdsbufferid_t) id) <br>_Get data count in SDS buffer stream._  |
|  [**sdsBufferId\_t**](group__SDS__Circular__Buffer.md#typedef-sdsbufferid_t) | [**sdsBufferOpen**](#function-sdsbufferopen) (void \* buf, uint32\_t buf\_size, uint32\_t threshold\_low, uint32\_t threshold\_high) <br>_Open SDS buffer stream._  |
|  uint32\_t | [**sdsBufferRead**](#function-sdsbufferread) ([**sdsBufferId\_t**](group__SDS__Circular__Buffer.md#typedef-sdsbufferid_t) id, void \* buf, uint32\_t buf\_size) <br>_Read data from SDS buffer stream._  |
|  int32\_t | [**sdsBufferRegisterEvents**](#function-sdsbufferregisterevents) ([**sdsBufferId\_t**](group__SDS__Circular__Buffer.md#typedef-sdsbufferid_t) id, [**sdsBufferEvent\_t**](group__SDS__Circular__Buffer.md#typedef-sdsbufferevent_t) event\_cb, uint32\_t event\_mask, void \* event\_arg) <br>_Register SDS buffer stream events._  |
|  uint32\_t | [**sdsBufferWrite**](#function-sdsbufferwrite) ([**sdsBufferId\_t**](group__SDS__Circular__Buffer.md#typedef-sdsbufferid_t) id, const void \* buf, uint32\_t buf\_size) <br>_Write data to SDS buffer stream._  |




























## Detailed Description


The stream data is written to and read from a circular buffer allocated in RAM. Event callback function which is triggered on data threshold reached can be registered. 


    
## Public Types Documentation




### typedef sdsBufferEvent\_t 

_Callback function for SDS circular buffer handling._ 
```
typedef void(* sdsBufferEvent_t) (sdsBufferId_t id, uint32_t event, void *arg);
```



todo




**Parameters:**


* `id` [**sdsBufferId\_t**](group__SDS__Circular__Buffer.md#typedef-sdsbufferid_t) handle to SDS buffer stream 
* `event` event code 
* `arg` pointer to argument 




        

<hr>



### typedef sdsBufferId\_t 

_Handle to SDS buffer stream._ 
```
typedef void* sdsBufferId_t;
```



todo test 


        

<hr>
## Public Functions Documentation




### function sdsBufferClear 

_Clear SDS buffer stream data._ 
```
int32_t sdsBufferClear (
    sdsBufferId_t id
) 
```



todo




**Parameters:**


* `id` [**sdsBufferId\_t**](group__SDS__Circular__Buffer.md#typedef-sdsbufferid_t) handle to SDS buffer stream 



**Returns:**

return code 





        

<hr>



### function sdsBufferClose 

_Close SDS buffer stream._ 
```
int32_t sdsBufferClose (
    sdsBufferId_t id
) 
```



todo




**Parameters:**


* `id` [**sdsBufferId\_t**](group__SDS__Circular__Buffer.md#typedef-sdsbufferid_t) handle to SDS buffer stream 



**Returns:**

return code 





        

<hr>



### function sdsBufferGetCount 

_Get data count in SDS buffer stream._ 
```
uint32_t sdsBufferGetCount (
    sdsBufferId_t id
) 
```



todo




**Parameters:**


* `id` [**sdsBufferId\_t**](group__SDS__Circular__Buffer.md#typedef-sdsbufferid_t) handle to SDS buffer stream 



**Returns:**

number of bytes in buffer stream 





        

<hr>



### function sdsBufferOpen 

_Open SDS buffer stream._ 
```
sdsBufferId_t sdsBufferOpen (
    void * buf,
    uint32_t buf_size,
    uint32_t threshold_low,
    uint32_t threshold_high
) 
```



todo




**Parameters:**


* `buf` pointer to buffer for stream 
* `buf_size` buffer size in bytes 
* `threshold_low` data low threshold in bytes 
* `threshold_high` data high threshold in bytes 



**Returns:**

[**sdsBufferId\_t**](group__SDS__Circular__Buffer.md#typedef-sdsbufferid_t) Handle to SDS buffer stream 





        

<hr>



### function sdsBufferRead 

_Read data from SDS buffer stream._ 
```
uint32_t sdsBufferRead (
    sdsBufferId_t id,
    void * buf,
    uint32_t buf_size
) 
```



todo




**Parameters:**


* `id` [**sdsBufferId\_t**](group__SDS__Circular__Buffer.md#typedef-sdsbufferid_t) handle to SDS buffer stream 
* `buf` pointer to buffer for data to read 
* `buf_size` buffer size in bytes 



**Returns:**

number of bytes read 





        

<hr>



### function sdsBufferRegisterEvents 

_Register SDS buffer stream events._ 
```
int32_t sdsBufferRegisterEvents (
    sdsBufferId_t id,
    sdsBufferEvent_t event_cb,
    uint32_t event_mask,
    void * event_arg
) 
```



todo




**Parameters:**


* `id` [**sdsBufferId\_t**](group__SDS__Circular__Buffer.md#typedef-sdsbufferid_t) handle to SDS buffer stream 
* `event_cb` pointer to sdsEvent\_t 
* `event_mask` event mask 
* `event_arg` event argument 



**Returns:**

return code 





        

<hr>



### function sdsBufferWrite 

_Write data to SDS buffer stream._ 
```
uint32_t sdsBufferWrite (
    sdsBufferId_t id,
    const void * buf,
    uint32_t buf_size
) 
```



todo




**Parameters:**


* `id` [**sdsBufferId\_t**](group__SDS__Circular__Buffer.md#typedef-sdsbufferid_t) handle to SDS buffer stream 
* `buf` pointer to buffer with data to write 
* `buf_size` buffer size in bytes 



**Returns:**

number of bytes written 





        

<hr>

------------------------------


