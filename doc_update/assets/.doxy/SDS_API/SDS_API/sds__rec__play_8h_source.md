

# File sds\_rec\_play.h

[**File List**](files.md) **>** [**include**](dir_d09908635ef304ba819d3349bcb716bf.md) **>** [**sds\_rec\_play.h**](sds__rec__play_8h.md)

[Go to the documentation of this file](sds__rec__play_8h.md)


```C++
/*
 * Copyright (c) 2025 Arm Limited. All rights reserved.
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

#ifndef SDS_REC_PLAY_H
#define SDS_REC_PLAY_H

#ifdef  __cplusplus
extern "C"
{
#endif

#include <stdint.h>

// ==== SDS Recorder and Player ====

typedef void *sdsRecPlayId_t;                   // Handle to SDS Recorder/Player stream

// Function return codes
#define SDS_REC_PLAY_OK                 (0)     // Operation completed successfully
#define SDS_REC_PLAY_ERROR              (-1)    // Operation failed
#define SDS_REC_PLAY_ERROR_PARAMETER    (-2)    // Operation failed: parameter error
#define SDS_REC_PLAY_ERROR_TIMEOUT      (-3)    // Operation failed: timeout error
#define SDS_REC_PLAY_ERROR_IO           (-4)    // Operation failed: SDS I/O interface error
#define SDS_REC_ERROR_NO_SPACE          (-5)    // Operation failed: insufficient space in stream buffer
#define SDS_PLAY_ERROR_NO_DATA          (-6)    // Operation failed: insufficient data in stream buffer
#define SDS_PLAY_EOS                    (-7)    // End of stream reached

// Event codes
#define SDS_REC_PLAY_EVENT_ERROR_IO     (1UL)   // SDS I/O interface error
#define SDS_REC_EVENT_ERROR_NO_SPACE    (2UL)   // sdsRecWrite() failed: insufficient space in stream buffer
#define SDS_PLAY_EVENT_ERROR_NO_DATA    (4UL)   // sdsPlayRead() failed: insufficient data in stream buffer

typedef void (*sdsRecPlayEvent_t) (sdsRecPlayId_t id, uint32_t event);

int32_t sdsRecPlayInit (sdsRecPlayEvent_t event_cb);

int32_t sdsRecPlayUninit (void);

sdsRecPlayId_t sdsRecOpen (const char *name, void *buf, uint32_t buf_size);

int32_t sdsRecClose (sdsRecPlayId_t id);

int32_t sdsRecWrite (sdsRecPlayId_t id, uint32_t timestamp, const void *buf, uint32_t buf_size);

sdsRecPlayId_t sdsPlayOpen (const char *name, void *buf, uint32_t buf_size);

int32_t sdsPlayClose (sdsRecPlayId_t id);

int32_t sdsPlayRead (sdsRecPlayId_t id, uint32_t *timestamp, void *buf, uint32_t buf_size);

int32_t sdsPlayGetSize (sdsRecPlayId_t id);

#ifdef  __cplusplus
}
#endif

#endif  /* SDS_REC_PLAY_H */
```


