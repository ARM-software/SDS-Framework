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

#ifndef SDS_CONTROL_H_
#define SDS_CONTROL_H_

#include <stdint.h>
#include "cmsis_compiler.h"

#ifdef  __cplusplus
extern "C"
{
#endif

#include "sds_rec_play.h"

// SDS streaming states
#define SDS_STREAMING_INACTIVE    0     // Streaming is not active
#define SDS_STREAMING_ACTIVE      1     // Streaming is active, SDS streams are open and ready for read/write operations
#define SDS_STREAMING_STOP        2     // Request to stop streaming and close the open streams
#define SDS_STREAMING_STOP_SAFE   3     // Safe state for streaming to be stopped

// Assert macro
#define SDS_ASSERT(cond)                \
  if (!(cond) && (!sdsError.occurred)) {\
    sdsError.occurred = 1U;             \
    sdsError.file = __FILE__;           \
    sdsError.line = __LINE__;           \
  }

// Error information structure
typedef struct {
  uint8_t occurred;
  uint8_t reported;
  const char *file;
  uint32_t line;
} sdsError_t;

// Error information
extern sdsError_t sdsError;

// SDS IO active status
extern volatile uint8_t sdsStreamingState;

// SDS control thread function
extern __NO_RETURN void sdsControlThread (void *argument);

#ifdef  __cplusplus
}
#endif

#endif
