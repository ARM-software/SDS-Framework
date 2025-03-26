

# Group SDS\_Circular\_Buffer



[**Modules**](modules.md) **>** [**SDS\_Circular\_Buffer**](group__SDS__Circular__Buffer.md)



sds.h _: SDS circular buffer handling for data streams_[More...](#detailed-description)


















## Public Types

| Type | Name |
| ---: | :--- |
| typedef void \* | [**sdsId\_t**](#typedef-sdsid_t)  <br>_Handle to SDS buffer stream._  |




















## Public Functions

| Type | Name |
| ---: | :--- |
|  int32\_t | [**sdsClear**](#function-sdsclear) ([**sdsId\_t**](group__SDS__Circular__Buffer.md#typedef-sdsid_t) id) <br>_Clear buffer stream data._  |
|  int32\_t | [**sdsClose**](#function-sdsclose) ([**sdsId\_t**](group__SDS__Circular__Buffer.md#typedef-sdsid_t) id) <br>_Close buffer stream._  |
|  uint32\_t | [**sdsGetCount**](#function-sdsgetcount) ([**sdsId\_t**](group__SDS__Circular__Buffer.md#typedef-sdsid_t) id) <br>_Get data count in buffer stream._  |
|  uint32\_t | [**sdsRead**](#function-sdsread) ([**sdsId\_t**](group__SDS__Circular__Buffer.md#typedef-sdsid_t) id, void \* buf, uint32\_t buf\_size) <br>_Read data from buffer stream._  |
|  int32\_t | [**sdsRegisterEvents**](#function-sdsregisterevents) ([**sdsId\_t**](group__SDS__Circular__Buffer.md#typedef-sdsid_t) id, sdsEvent\_t event\_cb, uint32\_t event\_mask, void \* event\_arg) <br>_Register buffer stream events._  |
|  uint32\_t | [**sdsWrite**](#function-sdswrite) ([**sdsId\_t**](group__SDS__Circular__Buffer.md#typedef-sdsid_t) id, const void \* buf, uint32\_t buf\_size) <br>_Write data to buffer stream._  |




























## Detailed Description


The stream data is written to and read from a circular buffer allocated in RAM. Event callback function which is triggered on data threshold reached can be registered. 


    
## Public Types Documentation




### typedef sdsId\_t 

_Handle to SDS buffer stream._ 
```
typedef void* sdsId_t;
```



todo test 


        

<hr>
## Public Functions Documentation




### function sdsClear 

_Clear buffer stream data._ 
```
int32_t sdsClear (
    sdsId_t id
) 
```





**Parameters:**


* `id` [**sdsId\_t**](group__SDS__Circular__Buffer.md#typedef-sdsid_t) handle to SDS buffer stream 



**Returns:**

return code


todo 


        

<hr>



### function sdsClose 

_Close buffer stream._ 
```
int32_t sdsClose (
    sdsId_t id
) 
```





**Parameters:**


* `id` [**sdsId\_t**](group__SDS__Circular__Buffer.md#typedef-sdsid_t) handle to SDS buffer stream 



**Returns:**

return code


todo 


        

<hr>



### function sdsGetCount 

_Get data count in buffer stream._ 
```
uint32_t sdsGetCount (
    sdsId_t id
) 
```





**Parameters:**


* `id` [**sdsId\_t**](group__SDS__Circular__Buffer.md#typedef-sdsid_t) handle to SDS buffer stream 



**Returns:**

number of bytes in buffer stream


todo 


        

<hr>



### function sdsRead 

_Read data from buffer stream._ 
```
uint32_t sdsRead (
    sdsId_t id,
    void * buf,
    uint32_t buf_size
) 
```





**Parameters:**


* `id` [**sdsId\_t**](group__SDS__Circular__Buffer.md#typedef-sdsid_t) handle to SDS buffer stream 
* `buf` pointer to buffer for data to read 
* `buf_size` buffer size in bytes 



**Returns:**

number of bytes read


todo 


        

<hr>



### function sdsRegisterEvents 

_Register buffer stream events._ 
```
int32_t sdsRegisterEvents (
    sdsId_t id,
    sdsEvent_t event_cb,
    uint32_t event_mask,
    void * event_arg
) 
```





**Parameters:**


* `id` [**sdsId\_t**](group__SDS__Circular__Buffer.md#typedef-sdsid_t) handle to SDS buffer stream 
* `event_cb` pointer to sdsEvent\_t 
* `event_mask` event mask 
* `event_arg` event argument 



**Returns:**

return code


todo 


        

<hr>



### function sdsWrite 

_Write data to buffer stream._ 
```
uint32_t sdsWrite (
    sdsId_t id,
    const void * buf,
    uint32_t buf_size
) 
```





**Parameters:**


* `id` [**sdsId\_t**](group__SDS__Circular__Buffer.md#typedef-sdsid_t) handle to SDS buffer stream 
* `buf` pointer to buffer with data to write 
* `buf_size` buffer size in bytes 



**Returns:**

number of bytes written


todo 


        

<hr>

------------------------------


