

# File sdsio.h

[**File List**](files.md) **>** [**include**](dir_3b29f9c6a44e933bcd0b782bd8b86dc3.md) **>** [**sdsio.h**](sdsio_8h.md)

[Go to the documentation of this file](sdsio_8h.md)


```C++
/*
 * Copyright (c) 2022-2026 Arm Limited. All rights reserved.
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

typedef void *sdsioId_t;        // Handle to SDS I/O stream

// Open Mode
typedef enum {
  sdsioModeRead  = 0,           // Open for read (binary)
  sdsioModeWrite = 1            // Open for write (binary)
} sdsioMode_t;                  // Open mode (read/write)

int32_t sdsioInit (void);

int32_t sdsioUninit (void);

sdsioId_t sdsioOpen (const char *name, sdsioMode_t mode);

int32_t sdsioClose (sdsioId_t id);

int32_t sdsioWrite (sdsioId_t id, const void *buf, uint32_t buf_size);

int32_t sdsioRead (sdsioId_t id, void *buf, uint32_t buf_size);

#ifdef  __cplusplus
}
#endif

#endif  /* SDSIO_H */
```


