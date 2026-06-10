

# File sds.h

[**File List**](files.md) **>** [**include**](dir_d09908635ef304ba819d3349bcb716bf.md) **>** [**sds.h**](sds_8h.md)

[Go to the documentation of this file](sds_8h.md)


```C++
/*
 * Copyright (c) 2025-2026 Arm Limited. All rights reserved.
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

#include <stdio.h>
#include <stdint.h>

// ==== SDS Interface ====

typedef void *sdsId_t;                  // Handle to SDS stream

// Function return codes
#define SDS_OK                  (0)     
#define SDS_ERROR               (-1)    
#define SDS_ERROR_PARAMETER     (-2)    
#define SDS_ERROR_TIMEOUT       (-3)    
#define SDS_ERROR_IO            (-4)    
#define SDS_NO_SPACE            (-5)    
#define SDS_NO_DATA             (-6)    
#define SDS_EOS                 (-7)    

// Event codes for sdsEvent callback function
#define SDS_EVENT_ERROR_IO      (1UL)   
#define SDS_EVENT_NO_SPACE      (2UL)   
#define SDS_EVENT_NO_DATA       (4UL)   

// SDS stream open mode
typedef enum {
  sdsModeRead  = 0,                     // Open SDS stream for read (binary)
  sdsModeWrite = 1                      // Open SDS stream for write (binary)
} sdsMode_t;

typedef void (*sdsEvent_t) (sdsId_t id, uint32_t event);

int32_t sdsInit (sdsEvent_t event_cb);

int32_t sdsUninit (void);

sdsId_t sdsOpen (const char *name, sdsMode_t mode, void *buf, uint32_t buf_size);

int32_t sdsClose (sdsId_t id);

int32_t sdsWrite (sdsId_t id, uint32_t timeslot, const void *buf, uint32_t buf_size);

int32_t sdsRead (sdsId_t id, uint32_t *timeslot, void *buf, uint32_t buf_size);

int32_t sdsGetSize (sdsId_t id);


// ==== SDS Control Interface ====

#ifndef SDS_STDIO
#define SDS_STDIO               1
#endif

#ifndef SDS_PRINTF

#if     SDS_STDIO == 1
// Print messages to STDIO
#define SDS_PRINTF(...)                         \
  printf(__VA_ARGS__)
#else
#define SDS_PRINTF(...)
#endif

#endif

// Error information structure
typedef struct {                    
  int32_t status;                       
  const char *file;                     
  uint32_t line;                        
  uint8_t occurred;                     
} sdsError_t;

// Global error information
extern sdsError_t sdsError;

#ifndef SDS_ERROR_CHECK

// Check for error and record error location
#define SDS_ERROR_CHECK(sds_status)                             \
  if ((sds_status != SDS_OK) && (sdsError.occurred == 0U)) {    \
    sdsError.status = sds_status;                               \
    sdsError.file = __FILE__;                                   \
    sdsError.line = __LINE__;                                   \
    sdsError.occurred = 1U;                                     \
    SDS_PRINTF("Error: SDS_ERROR_CHECK status = %i: %s:%u\n", sds_status, sdsError.file, sdsError.line); \
  }

#endif

#ifndef SDS_ASSERT

// Assert macro
#define SDS_ASSERT(cond)                        \
  if (!(cond) && (sdsError.occurred == 0U)) {   \
    sdsError.status = SDS_OK;                   \
    sdsError.file = __FILE__;                   \
    sdsError.line = __LINE__;                   \
    sdsError.occurred = 1U;                     \
    SDS_PRINTF("Error: SDS_ASSERT failed: %s:%u\n", sdsError.file, sdsError.line); \
  }

#endif

// sdsFlags bitmask definitions
#define SDS_FLAG_START     (0x80000000UL)   
#define SDS_FLAG_TERMINATE (0x40000000UL)   
#define SDS_FLAG_PLAYBACK  (0x20000000UL)   
#define SDS_FLAG_ALIVE     (0x10000000UL)   
#define SDS_FLAG_RESET     (0x08000000UL)   
                                            // Bits 24..26 are reserved for future enhancements
                                            // Bits 0..23 for used for user options (i.e. bypassing filter, etc.)

// Configuration options and control information
extern volatile uint32_t sdsFlags;

// sdsState value definitions
#define SDS_STATE_INACTIVE      (0UL)   
#define SDS_STATE_CONNECTED     (1UL)   
#define SDS_STATE_START         (2UL)   
#define SDS_STATE_ACTIVE        (3UL)   
#define SDS_STATE_STOP_REQ      (4UL)   
#define SDS_STATE_STOP_DONE     (5UL)   
#define SDS_STATE_END           (6UL)   
#define SDS_STATE_RESET         (7UL)   
#define SDS_STATE_TERMINATE     (8UL)   

// State information
extern volatile uint32_t sdsState;

// Idle rate information
extern volatile uint32_t sdsIdleRate;

int32_t sdsExchange (void);

void sdsFlagsModify (uint32_t set_mask, uint32_t clear_mask);

#ifdef  __cplusplus
}
#endif

#endif  /* SDS_H */
```


