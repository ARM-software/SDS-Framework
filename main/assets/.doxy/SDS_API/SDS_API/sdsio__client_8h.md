

# File sdsio\_client.h



[**FileList**](files.md) **>** [**include**](dir_d09908635ef304ba819d3349bcb716bf.md) **>** [**sdsio\_client.h**](sdsio__client_8h.md)

[Go to the source code of this file](sdsio__client_8h_source.md)


















## Classes

| Type | Name |
| ---: | :--- |
| struct | [**header\_t**](structheader__t.md) <br> |






















## Public Functions

| Type | Name |
| ---: | :--- |
|  int32\_t | [**sdsioClientInit**](#function-sdsioclientinit) (void) <br>_Initialize SDS I/O Client._  |
|  uint32\_t | [**sdsioClientReceive**](#function-sdsioclientreceive) ([**header\_t**](structheader__t.md) \* header, void \* data, uint32\_t data\_size) <br>_Receive data from SDSIO-Server._  |
|  uint32\_t | [**sdsioClientSend**](#function-sdsioclientsend) (const [**header\_t**](structheader__t.md) \* header, const void \* data, uint32\_t data\_size) <br>_Send data to SDSIO-Server._  |
|  int32\_t | [**sdsioClientUninit**](#function-sdsioclientuninit) (void) <br>_Un-Initialize SDS I/O Client._  |



























## Macros

| Type | Name |
| ---: | :--- |
| define  | [**SDSIO\_CMD\_CLOSE**](sdsio__client_8h.md#define-sdsio_cmd_close)  `2U`<br> |
| define  | [**SDSIO\_CMD\_EOS**](sdsio__client_8h.md#define-sdsio_cmd_eos)  `5U`<br> |
| define  | [**SDSIO\_CMD\_OPEN**](sdsio__client_8h.md#define-sdsio_cmd_open)  `1U`<br> |
| define  | [**SDSIO\_CMD\_READ**](sdsio__client_8h.md#define-sdsio_cmd_read)  `4U`<br> |
| define  | [**SDSIO\_CMD\_WRITE**](sdsio__client_8h.md#define-sdsio_cmd_write)  `3U`<br> |

## Public Functions Documentation




### function sdsioClientInit 

_Initialize SDS I/O Client._ 
```C++
int32_t sdsioClientInit (
    void
) 
```





**Returns:**

SDIOS\_OK: initialization success SDSIO\_ERROR: initialization failed 





        

<hr>



### function sdsioClientReceive 

_Receive data from SDSIO-Server._ 
```C++
uint32_t sdsioClientReceive (
    header_t * header,
    void * data,
    uint32_t data_size
) 
```





**Parameters:**


* `header` pointer to header 
* `data` pointer to buffer for data to read 
* `data_size` data size in bytes 



**Returns:**

number of bytes received (including header) 





        

<hr>



### function sdsioClientSend 

_Send data to SDSIO-Server._ 
```C++
uint32_t sdsioClientSend (
    const header_t * header,
    const void * data,
    uint32_t data_size
) 
```





**Parameters:**


* `header` pointer to header 
* `data` pointer to buffer with data to send 
* `data_size` data size in bytes 



**Returns:**

number of bytes sent (including header) 





        

<hr>



### function sdsioClientUninit 

_Un-Initialize SDS I/O Client._ 
```C++
int32_t sdsioClientUninit (
    void
) 
```





**Returns:**

SDIOS\_OK: un-initialization success SDSIO\_ERROR: un-initialization failed 





        

<hr>
## Macro Definition Documentation





### define SDSIO\_CMD\_CLOSE 

```C++
#define SDSIO_CMD_CLOSE `2U`
```




<hr>



### define SDSIO\_CMD\_EOS 

```C++
#define SDSIO_CMD_EOS `5U`
```




<hr>



### define SDSIO\_CMD\_OPEN 

```C++
#define SDSIO_CMD_OPEN `1U`
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

