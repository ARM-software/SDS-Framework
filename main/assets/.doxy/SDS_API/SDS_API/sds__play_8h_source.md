

# File sds\_play.h

[**File List**](files.md) **>** [**include**](dir_d09908635ef304ba819d3349bcb716bf.md) **>** [**sds\_play.h**](sds__play_8h.md)

[Go to the documentation of this file](sds__play_8h.md)


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

#ifndef SDS_PLAY_H
#define SDS_PLAY_H

#ifdef  __cplusplus
extern "C"
{
#endif

#include <stdint.h>

// ==== SDS Player ====

typedef void *sdsPlayId_t;                  

#define SDS_PLAY_OK             (0)         
#define SDS_PLAY_ERROR          (-1)        

#define SDS_PLAY_EVENT_IO_ERROR  (1UL << 0) 

typedef void (*sdsPlayEvent_t) (sdsPlayId_t id, uint32_t event);

int32_t sdsPlayInit (sdsPlayEvent_t event_cb);

int32_t sdsPlayUninit (void);

sdsPlayId_t sdsPlayOpen (const char *name, void *buf, uint32_t buf_size, uint32_t io_threshold);

int32_t sdsPlayClose (sdsPlayId_t id);

uint32_t sdsPlayRead (sdsPlayId_t id, uint32_t *timestamp, void *buf, uint32_t buf_size);

uint32_t sdsPlayGetSize (sdsPlayId_t id);

int32_t sdsPlayEndOfStream (sdsPlayId_t id);

#ifdef  __cplusplus
}
#endif

#endif  /* SDS_PLAY_H */
```


