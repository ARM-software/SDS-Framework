

# Struct sdsError\_t



[**ClassList**](annotated.md) **>** [**sdsError\_t**](structsdsError__t.md)


























## Public Attributes

| Type | Name |
| ---: | :--- |
|  const char \* | [**file**](#variable-file)  <br>_Pointer to the filename in which the error occurred._  |
|  uint32\_t | [**line**](#variable-line)  <br>_Line number at which the error occurred._  |
|  uint8\_t | [**occurred**](#variable-occurred)  <br>_Flag indicating that an error has occurred._  |
|  int32\_t | [**status**](#variable-status)  <br>_The error status code (see_ [_**Function Return Codes**_](group__SDS__Return__Codes.md) _)._ |












































## Public Attributes Documentation




### variable file 

_Pointer to the filename in which the error occurred._ 
```C++
const char* sdsError_t::file;
```




<hr>



### variable line 

_Line number at which the error occurred._ 
```C++
uint32_t sdsError_t::line;
```




<hr>



### variable occurred 

_Flag indicating that an error has occurred._ 
```C++
uint8_t sdsError_t::occurred;
```




<hr>



### variable status 

_The error status code (see_ [_**Function Return Codes**_](group__SDS__Return__Codes.md) _)._
```C++
int32_t sdsError_t::status;
```




<hr>

------------------------------
The documentation for this class was generated from the following file `sds/include/sds.h`

