

# File sdsio\_client.h



[**FileList**](files.md) **>** [**include**](dir_d09908635ef304ba819d3349bcb716bf.md) **>** [**sdsio\_client.h**](sdsio__client_8h.md)

[Go to the source code of this file](sdsio__client_8h_source.md)



* `#include <stdint.h>`















## Classes

| Type | Name |
| ---: | :--- |
| struct | [**header\_t**](structheader__t.md) <br> |






















## Public Functions

| Type | Name |
| ---: | :--- |
|  int32\_t | [**sdsioClientInit**](#function-sdsioclientinit) (void) <br>_Initialize SDSIO Client and ping SDSIO Server to verify connection._  |
|  int32\_t | [**sdsioClientReceive**](#function-sdsioclientreceive) (uint8\_t \* buf, uint32\_t buf\_size) <br>_Receive data from SDSIO-Server._  |
|  int32\_t | [**sdsioClientSend**](#function-sdsioclientsend) (const uint8\_t \* buf, uint32\_t buf\_size) <br>_Send data to SDSIO-Server._  |
|  int32\_t | [**sdsioClientUninit**](#function-sdsioclientuninit) (void) <br>_Un-Initialize SDSIO Client._  |



























## Macros

| Type | Name |
| ---: | :--- |
| define  | [**SDSIO\_CMD\_CLOSE**](sdsio__client_8h.md#define-sdsio_cmd_close)  `2U`<br> |
| define  | [**SDSIO\_CMD\_OPEN**](sdsio__client_8h.md#define-sdsio_cmd_open)  `1U`<br> |
| define  | [**SDSIO\_CMD\_PING**](sdsio__client_8h.md#define-sdsio_cmd_ping)  `5U`<br> |
| define  | [**SDSIO\_CMD\_READ**](sdsio__client_8h.md#define-sdsio_cmd_read)  `4U`<br> |
| define  | [**SDSIO\_CMD\_WRITE**](sdsio__client_8h.md#define-sdsio_cmd_write)  `3U`<br> |

## Public Functions Documentation




### function sdsioClientInit 

_Initialize SDSIO Client and ping SDSIO Server to verify connection._ 
```C++
int32_t sdsioClientInit (
    void
) 
```





**Returns:**

SDSIO\_OK on success or a negative value on error (see [**Function Return Codes**](group__SDS__IO__Return__Codes.md)) 





        

<hr>



### function sdsioClientReceive 

_Receive data from SDSIO-Server._ 
```C++
int32_t sdsioClientReceive (
    uint8_t * buf,
    uint32_t buf_size
) 
```





**Parameters:**


* `buf` pointer to buffer for data to read 
* `buf_size` buffer size in bytes 



**Returns:**

number of bytes successfully received or a negative value on error (see [**Function Return Codes**](group__SDS__IO__Return__Codes.md)) 





        

<hr>



### function sdsioClientSend 

_Send data to SDSIO-Server._ 
```C++
int32_t sdsioClientSend (
    const uint8_t * buf,
    uint32_t buf_size
) 
```





**Parameters:**


* `buf` pointer to buffer with data to send 
* `buf_size` buffer size in bytes 



**Returns:**

number of bytes successfully sent or a negative value on error (see [**Function Return Codes**](group__SDS__IO__Return__Codes.md)) 





        

<hr>



### function sdsioClientUninit 

_Un-Initialize SDSIO Client._ 
```C++
int32_t sdsioClientUninit (
    void
) 
```





**Returns:**

SDSIO\_OK on success or a negative value on error (see [**Function Return Codes**](group__SDS__IO__Return__Codes.md)) 





        

<hr>
## Macro Definition Documentation





### define SDSIO\_CMD\_CLOSE 

```C++
#define SDSIO_CMD_CLOSE `2U`
```




<hr>



### define SDSIO\_CMD\_OPEN 

```C++
#define SDSIO_CMD_OPEN `1U`
```




<hr>



### define SDSIO\_CMD\_PING 

```C++
#define SDSIO_CMD_PING `5U`
```




<hr>



### define SDSIO\_CMD\_READ 

```C++
#define SDSIO_CMD_READ `4U`
```




<hr>



### define SDSIO\_CMD\_WRITE 

```C++
#define SDSIO_CMD_WRITE `3U`
```




<hr>

------------------------------
The documentation for this class was generated from the following file `sds/include/sdsio_client.h`

