

# Group SDS\_Buffer



[**Modules**](modules.md) **>** [**SDS\_Buffer**](group__SDS__Buffer.md)



sds\_buffer.h _: SDS circular buffer handling for data streams_[More...](#detailed-description)












## Modules

| Type | Name |
| ---: | :--- |
| module | [**Event Codes**](group__SDS__Buffer__Event__Codes.md) <br>_SDS Buffer Event Codes._  |
| module | [**Function Return Codes**](group__SDS__Buffer__Return__Codes.md) <br>_SDS Buffer Function Return Codes._  |






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
|  int32\_t | [**sdsBufferGetCount**](#function-sdsbuffergetcount) ([**sdsBufferId\_t**](group__SDS__Buffer.md#typedef-sdsbufferid_t) id) <br>_Get data count in SDS buffer stream._  |
|  [**sdsBufferId\_t**](group__SDS__Buffer.md#typedef-sdsbufferid_t) | [**sdsBufferOpen**](#function-sdsbufferopen) (void \* buf, uint32\_t buf\_size, uint32\_t threshold\_low, uint32\_t threshold\_high) <br>_Open SDS buffer stream._  |
|  int32\_t | [**sdsBufferRead**](#function-sdsbufferread) ([**sdsBufferId\_t**](group__SDS__Buffer.md#typedef-sdsbufferid_t) id, void \* buf, uint32\_t buf\_size) <br>_Read data from SDS buffer stream._  |
|  int32\_t | [**sdsBufferRegisterEvents**](#function-sdsbufferregisterevents) ([**sdsBufferId\_t**](group__SDS__Buffer.md#typedef-sdsbufferid_t) id, [**sdsBufferEvent\_t**](group__SDS__Buffer.md#typedef-sdsbufferevent_t) event\_cb, uint32\_t event\_mask, void \* event\_arg) <br>_Register SDS buffer stream event callback function._  |
|  int32\_t | [**sdsBufferWrite**](#function-sdsbufferwrite) ([**sdsBufferId\_t**](group__SDS__Buffer.md#typedef-sdsbufferid_t) id, const void \* buf, uint32\_t buf\_size) <br>_Write data to SDS buffer stream._  |




























## Detailed Description


## Public Types Documentation




### typedef sdsBufferEvent\_t 

_Callback function for SDS circular buffer event handling._ 
```
typedef void(* sdsBufferEvent_t) (sdsBufferId_t id, uint32_t event, void *arg);
```



This function is registered by passing a pointer to it as a parameter to the [**sdsBufferRegisterEvents**](group__SDS__Buffer.md#function-sdsbufferregisterevents) function. It is invoked when the circular buffer either reaches or exceeds the high data threshold or falls to or below the low data threshold. The high and low data thresholds are configured using the [**sdsBufferOpen**](group__SDS__Buffer.md#function-sdsbufferopen) function.




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

SDS\_BUFFER\_OK on success or a negative value on error (see [**Function Return Codes**](group__SDS__Buffer__Return__Codes.md)) 





        

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

SDS\_BUFFER\_OK on success or a negative value on error (see [**Function Return Codes**](group__SDS__Buffer__Return__Codes.md)) 





        

<hr>



### function sdsBufferGetCount 

_Get data count in SDS buffer stream._ 
```
int32_t sdsBufferGetCount (
    sdsBufferId_t id
) 
```



Retrieves the number of data bytes currently available in the SDS circular buffer.




**Parameters:**


* `id` [**sdsBufferId\_t**](group__SDS__Buffer.md#typedef-sdsbufferid_t) handle to SDS buffer stream 



**Returns:**

number of data bytes available in buffer stream or a negative value on error (see [**Function Return Codes**](group__SDS__IO__Return__Codes.md)) 





        

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
int32_t sdsBufferRead (
    sdsBufferId_t id,
    void * buf,
    uint32_t buf_size
) 
```



Attempts to read up to `buf_size` bytes of data from the SDS circular buffer into a `buf`. If sufficient data is available, the requested number of bytes will be read. If only partial data is available, only the available bytes will be read. On success, the function returns the number of bytes actually read or 0 if SDS circular buffer is empty.




**Parameters:**


* `id` [**sdsBufferId\_t**](group__SDS__Buffer.md#typedef-sdsbufferid_t) handle to SDS buffer stream 
* `buf` pointer to buffer for data to read 
* `buf_size` buffer size in bytes 



**Returns:**

number of data bytes successfully read or a negative value on error (see [**Function Return Codes**](group__SDS__IO__Return__Codes.md)) 





        

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



Registers a [**sdsBufferEvent\_t**](group__SDS__Buffer.md#typedef-sdsbufferevent_t) callback function to handle threshold events for the specified SDS buffer.


The `event_mask` parameter specifies which buffer events should trigger the callback. It is a bitmask composed of values from the [**Event Codes**](group__SDS__Buffer__Event__Codes.md) enumeration:



* [**SDS\_BUFFER\_EVENT\_DATA\_LOW**](group__SDS__Buffer__Event__Codes.md#define-sds_buffer_event_data_low)
* [**SDS\_BUFFER\_EVENT\_DATA\_HIGH**](group__SDS__Buffer__Event__Codes.md#define-sds_buffer_event_data_high)




When an event matching the mask occurs, the registered `event_cb` function is invoked with `event_arg` as its context.


If `event_cb` is NULL, any previously registered callback for the specified buffer is unregistered.




**Parameters:**


* `id` [**sdsBufferId\_t**](group__SDS__Buffer.md#typedef-sdsbufferid_t) handle to SDS buffer stream 
* `event_cb` pointer to [**sdsBufferEvent\_t**](group__SDS__Buffer.md#typedef-sdsbufferevent_t) callback function, NULL to un-register 
* `event_mask` event mask 
* `event_arg` pointer to event argument 



**Returns:**

SDS\_BUFFER\_OK on success or a negative value on error (see [**Function Return Codes**](group__SDS__Buffer__Return__Codes.md)) 





        

<hr>



### function sdsBufferWrite 

_Write data to SDS buffer stream._ 
```
int32_t sdsBufferWrite (
    sdsBufferId_t id,
    const void * buf,
    uint32_t buf_size
) 
```



Attempts to write up to `buf_size` bytes of data from `buf` to the SDS circular buffer. If sufficient space is available in the buffer, all data will be written. If only partial space is available, only the number of bytes that fit will be written. No data is overwritten. On success, the function returns the number of bytes actually written.




**Parameters:**


* `id` [**sdsBufferId\_t**](group__SDS__Buffer.md#typedef-sdsbufferid_t) handle to SDS buffer stream 
* `buf` pointer to buffer with data to write 
* `buf_size` buffer size in bytes 



**Returns:**

number of data bytes successfully written or a negative value on error (see [**Function Return Codes**](group__SDS__Buffer__Return__Codes.md)) 





        

<hr>

------------------------------


