

# File sdsio.h

[**File List**](files.md) **>** [**include**](dir_d09908635ef304ba819d3349bcb716bf.md) **>** [**sdsio.h**](sdsio_8h.md)

[Go to the documentation of this file](sdsio_8h.md)


```C++
/*
 * Copyright (c) 2022-2023 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef SDSIO_H
#define SDSIO_H

#ifdef  __cplusplus
extern "C"
{
#endif

#include <stdint.h>

// ==== Synchronous Data Stream Input/Output (SDS I/O) ====

typedef void *sdsioId_t;        

typedef enum {
  sdsioModeRead  = 0,           
  sdsioModeWrite = 1            
} sdsioMode_t;                  

#define SDSIO_OK                (0)         
#define SDSIO_ERROR             (-1)        

int32_t sdsioInit (void);

int32_t sdsioUninit (void);

sdsioId_t sdsioOpen (const char *name, sdsioMode_t mode);

int32_t sdsioClose (sdsioId_t id);

uint32_t sdsioWrite (sdsioId_t id, const void *buf, uint32_t buf_size);

uint32_t sdsioRead (sdsioId_t id, void *buf, uint32_t buf_size);

int32_t sdsioEndOfStream (sdsioId_t id);

#ifdef  __cplusplus
}
#endif

#endif  /* SDSIO_H */
```


