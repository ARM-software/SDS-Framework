

# File sds\_buffer.h

[**File List**](files.md) **>** [**include**](dir_d09908635ef304ba819d3349bcb716bf.md) **>** [**sds\_buffer.h**](sds__buffer_8h.md)

[Go to the documentation of this file](sds__buffer_8h.md)


```C++
/*
 * Copyright (c) 2022-2025 Arm Limited. All rights reserved.
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

#ifndef SDS_BUFFER_H
#define SDS_BUFFER_H

#ifdef  __cplusplus
extern "C"
{
#endif

#include <stdint.h>

// ==== Synchronous Data Stream (SDS) Buffer ====

typedef void *sdsBufferId_t;                        // Handle to SDS buffer stream

// Function return codes
#define SDS_BUFFER_OK                   (0)         // Operation completed successfully
#define SDS_BUFFER_ERROR                (-1)        // Operation failed

// Events
#define SDS_BUFFER_EVENT_DATA_LOW       (1UL)       // Data bellow or equal to low threshold
#define SDS_BUFFER_EVENT_DATA_HIGH      (2UL)       // Data above or equal to high threshold

typedef void (*sdsBufferEvent_t) (sdsBufferId_t id, uint32_t event, void *arg);

sdsBufferId_t sdsBufferOpen (void *buf, uint32_t buf_size, uint32_t threshold_low, uint32_t threshold_high);

int32_t sdsBufferClose (sdsBufferId_t id);

int32_t sdsBufferRegisterEvents (sdsBufferId_t id, sdsBufferEvent_t event_cb, uint32_t event_mask, void *event_arg);

uint32_t sdsBufferWrite (sdsBufferId_t id, const void *buf, uint32_t buf_size);

uint32_t sdsBufferRead (sdsBufferId_t id, void *buf, uint32_t buf_size);

int32_t sdsBufferClear (sdsBufferId_t id);

uint32_t sdsBufferGetCount (sdsBufferId_t id);

#ifdef  __cplusplus
}
#endif

#endif  /* SDS_BUFFER_H */
```


