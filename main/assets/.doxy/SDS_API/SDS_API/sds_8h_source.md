

# File sds.h

[**File List**](files.md) **>** [**include**](dir_d09908635ef304ba819d3349bcb716bf.md) **>** [**sds.h**](sds_8h.md)

[Go to the documentation of this file](sds_8h.md)


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

#ifndef SDS_H
#define SDS_H

#ifdef  __cplusplus
extern "C"
{
#endif

#include <stdint.h>

// ==== Synchronous Data Stream (SDS) ====

typedef void *sdsId_t;                      

#define SDS_OK                  (0)         
#define SDS_ERROR               (-1)        

#define SDS_EVENT_DATA_LOW      (1UL << 0)  
#define SDS_EVENT_DATA_HIGH     (1UL << 1)  

typedef void (*sdsEvent_t) (sdsId_t id, uint32_t event, void *arg);

sdsId_t sdsOpen (void *buf, uint32_t buf_size, uint32_t threshold_low, uint32_t threshold_high);

int32_t sdsClose (sdsId_t id);

int32_t sdsRegisterEvents (sdsId_t id, sdsEvent_t event_cb, uint32_t event_mask, void *event_arg);

uint32_t sdsWrite (sdsId_t id, const void *buf, uint32_t buf_size);

uint32_t sdsRead (sdsId_t id, void *buf, uint32_t buf_size);

int32_t sdsClear (sdsId_t id);

uint32_t sdsGetCount (sdsId_t id);

#ifdef  __cplusplus
}
#endif

#endif  /* SDS_H */
```


