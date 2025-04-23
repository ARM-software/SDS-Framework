

# Group SDS\_Buffer



[**Modules**](modules.md) **>** [**SDS\_Buffer**](group__SDS__Buffer.md)



sds\_buffer.h _: SDS circular buffer handling for data streams_[More...](#detailed-description)












## Modules

| Type | Name |
| ---: | :--- |
| module | [**Error Codes**](group__SDS__Buffer__Error__Codes.md) <br>_SDS Buffer Error Codes._  |
| module | [**Event Codes**](group__SDS__Buffer__Event__Codes.md) <br>_SDS Buffer Event Codes._  |






## Public Types

| Type | Name |
| ---: | :--- |
| typedef void(\* | [**sdsBufferEvent\_t**](#typedef-sdsbufferevent_t)  <br>_Callback function for SDS circular buffer event handling._  |
| typedef void \* | [**sdsBufferId\_t**](#typedef-sdsbufferid_t)  <br>_Handle to SDS circular buffer._  |




















## Public Functions

| Type | Name |
| ---: | :--- |
|  int32\_t | [**sdsBufferClear**](#function-sdsbufferclear) ([**sdsBufferId\_t**](group__SDS__Buffer.md#typedef-sdsbufferid_t) id) <br>_Clear SDS buffer stream data._  |
|  int32\_t | [**sdsBufferClose**](#function-sdsbufferclose) ([**sdsBufferId\_t**](group__SDS__Buffer.md#typedef-sdsbufferid_t) id) <br>_Close SDS buffer stream._  |
|  uint32\_t | [**sdsBufferGetCount**](#function-sdsbuffergetcount) ([**sdsBufferId\_t**](group__SDS__Buffer.md#typedef-sdsbufferid_t) id) <br>_Get data count in SDS buffer stream._  |
|  [**sdsBufferId\_t**](group__SDS__Buffer.md#typedef-sdsbufferid_t) | [**sdsBufferOpen**](#function-sdsbufferopen) (void \* buf, uint32\_t buf\_size, uint32\_t threshold\_low, uint32\_t threshold\_high) <br>_Open SDS buffer stream._  |
|  uint32\_t | [**sdsBufferRead**](#function-sdsbufferread) ([**sdsBufferId\_t**](group__SDS__Buffer.md#typedef-sdsbufferid_t) id, void \* buf, uint32\_t buf\_size) <br>_Read data from SDS buffer stream._  |
|  int32\_t | [**sdsBufferRegisterEvents**](#function-sdsbufferregisterevents) ([**sdsBufferId\_t**](group__SDS__Buffer.md#typedef-sdsbufferid_t) id, [**sdsBufferEvent\_t**](group__SDS__Buffer.md#typedef-sdsbufferevent_t) event\_cb, uint32\_t event\_mask, void \* event\_arg) <br>_Register SDS buffer stream event callback function._  |
|  uint32\_t | [**sdsBufferWrite**](#function-sdsbufferwrite) ([**sdsBufferId\_t**](group__SDS__Buffer.md#typedef-sdsbufferid_t) id, const void \* buf, uint32\_t buf\_size) <br>_Write data to SDS buffer stream._  |




























## Detailed Description


The **SDS** circular **buffer** facilitates efficient reading from and writing to a circular buffer allocated in RAM. A callback function can be registered to respond when data thresholds (high or low) are reached. 


    
## Public Types Documentation




### typedef sdsBufferEvent\_t 

_Callback function for SDS circular buffer event handling._ 
```
typedef void(* sdsBufferEvent_t) (sdsBufferId_t id, uint32_t event, void *arg);
```



This function is registered by passing a pointer to it as a parameter to the [**sdsBufferRegisterEvents**](group__SDS__Buffer.md#function-sdsbufferregisterevents) function. It is invoked when the circular buffer either exceeds the high data threshold or drops below the low data threshold. The high and low data thresholds are configured using the [**sdsBufferOpen**](group__SDS__Buffer.md#function-sdsbufferopen) function.




**Parameters:**


* `id` [**sdsBufferId\_t**](group__SDS__Buffer.md#typedef-sdsbufferid_t) handle to SDS buffer stream 
* `event` event code (see [**Event Codes**](group__SDS__Buffer__Event__Codes.md)) 
* `arg` pointer to argument registered with [**sdsBufferRegisterEvents**](group__SDS__Buffer.md#function-sdsbufferregisterevents) 




        

<hr>



### typedef sdsBufferId\_t 

_Handle to SDS circular buffer._ 
```
typedef void* sdsBufferId_t;
```



This _pointer_ defines the handle to SDS circular buffer. It is used to identify a circular buffer across the different functions. 


        

<hr>
## Public Functions Documentation




### function sdsBufferClear 

_Clear SDS buffer stream data._ 
```
int32_t sdsBufferClear (
    sdsBufferId_t id
) 
```



Clears any data from the SDS circular buffer and resets the circular buffer to empty state.




**Parameters:**


* `id` [**sdsBufferId\_t**](group__SDS__Buffer.md#typedef-sdsbufferid_t) handle to SDS buffer stream 



**Returns:**

return code (see [**Error Codes**](group__SDS__Buffer__Error__Codes.md)) 





        

<hr>



### function sdsBufferClose 

_Close SDS buffer stream._ 
```
int32_t sdsBufferClose (
    sdsBufferId_t id
) 
```



Closes the SDS circular buffer when read or write operations are no longer required.




**Parameters:**


* `id` [**sdsBufferId\_t**](group__SDS__Buffer.md#typedef-sdsbufferid_t) handle to SDS buffer stream 



**Returns:**

return code (see [**Error Codes**](group__SDS__Buffer__Error__Codes.md)) 





        

<hr>



### function sdsBufferGetCount 

_Get data count in SDS buffer stream._ 
```
uint32_t sdsBufferGetCount (
    sdsBufferId_t id
) 
```



Retrieves the number of data bytes currently available in the SDS circular buffer.




**Parameters:**


* `id` [**sdsBufferId\_t**](group__SDS__Buffer.md#typedef-sdsbufferid_t) handle to SDS buffer stream 



**Returns:**

number of data bytes available in buffer stream 





        

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



Opens the SDS circular buffer for read or write operations. The function returns the handle to the SDS buffer stream; if the buffer could not be opened, it returns NULL.




**Parameters:**


* `buf` pointer to buffer for stream 
* `buf_size` buffer size in bytes 
* `threshold_low` data low threshold in bytes 
* `threshold_high` data high threshold in bytes 



**Returns:**

[**sdsBufferId\_t**](group__SDS__Buffer.md#typedef-sdsbufferid_t) Handle to SDS buffer stream, or NULL if operation failed 





        

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



Reads data from the SDS circular buffer. If there is no data in the circular buffer the function will return 0.




**Parameters:**


* `id` [**sdsBufferId\_t**](group__SDS__Buffer.md#typedef-sdsbufferid_t) handle to SDS buffer stream 
* `buf` pointer to buffer for data to read 
* `buf_size` buffer size in bytes 



**Returns:**

number of data bytes read, or 0 if operation failed 





        

<hr>



### function sdsBufferRegisterEvents 

_Register SDS buffer stream event callback function._ 
```
int32_t sdsBufferRegisterEvents (
    sdsBufferId_t id,
    sdsBufferEvent_t event_cb,
    uint32_t event_mask,
    void * event_arg
) 
```



Registers a [**sdsBufferEvent\_t**](group__SDS__Buffer.md#typedef-sdsbufferevent_t) callback function for handling buffer threshold events.




**Parameters:**


* `id` [**sdsBufferId\_t**](group__SDS__Buffer.md#typedef-sdsbufferid_t) handle to SDS buffer stream 
* `event_cb` pointer to [**sdsBufferEvent\_t**](group__SDS__Buffer.md#typedef-sdsbufferevent_t) callback function 
* `event_mask` event mask 
* `event_arg` pointer to event argument 



**Returns:**

return code (see [**Error Codes**](group__SDS__Buffer__Error__Codes.md)) 





        

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



Writes data to the SDS circular buffer. If the buffer is full, no additional data will be written until space is freed by the [**sdsBufferRead**](group__SDS__Buffer.md#function-sdsbufferread) function. The function returns the number of bytes successfully written; if the buffer is full, it returns 0.




**Parameters:**


* `id` [**sdsBufferId\_t**](group__SDS__Buffer.md#typedef-sdsbufferid_t) handle to SDS buffer stream 
* `buf` pointer to buffer with data to write 
* `buf_size` buffer size in bytes 



**Returns:**

number of data bytes successfully written, or 0 if operation failed 





        

<hr>

------------------------------


