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
#define SDS_OK                  (0)     // Operation completed successfully
#define SDS_ERROR               (-1)    // Operation failed
#define SDS_ERROR_PARAMETER     (-2)    // Operation failed: parameter error
#define SDS_ERROR_TIMEOUT       (-3)    // Operation failed: timeout error
#define SDS_ERROR_IO            (-4)    // Operation failed: SDS I/O interface error
#define SDS_NO_SPACE            (-5)    // Operation failed: insufficient space in SDS stream buffer
#define SDS_NO_DATA             (-6)    // Operation failed: insufficient data in SDS stream buffer
#define SDS_EOS                 (-7)    // End of SDS stream reached

// Event codes for sdsEvent callback function
#define SDS_EVENT_ERROR_IO      (1UL)   // SDS I/O interface error
#define SDS_EVENT_NO_SPACE      (2UL)   // sdsWrite() failed: insufficient space in SDS stream buffer
#define SDS_EVENT_NO_DATA       (4UL)   // sdsRead() failed: insufficient data in SDS stream buffer

// SDS stream open mode
typedef enum {
  sdsModeRead  = 0,                     // Open SDS stream for read (binary)
  sdsModeWrite = 1                      // Open SDS stream for write (binary)
} sdsMode_t;

/**
  \typedef void (*sdsEvent_t) (sdsId_t id, uint32_t event)
  \brief       Callback function for SDS stream events.
  \param[in]   id             \ref sdsId_t handle of SDS stream
  \param[in]   event          event code (see \ref SDS_Event_Codes)
*/
typedef void (*sdsEvent_t) (sdsId_t id, uint32_t event);

/**
  \fn          int32_t sdsInit (sdsEvent_t event_cb)
  \brief       Initialize SDS.
  \param[in]   event_cb       pointer to \ref sdsEvent_t callback function
  \return      SDS_OK on success or
               a negative value on error (see \ref SDS_Return_Codes)
*/
int32_t sdsInit (sdsEvent_t event_cb);

/**
  \fn          int32_t sdsUninit (void)
  \brief       Uninitialize SDS.
  \return      SDS_OK on success or
               a negative value on error (see \ref SDS_Return_Codes)
*/
int32_t sdsUninit (void);

/**
  \fn          sdsId_t sdsOpen (const char *name, sdsMode_t mode, void *buf, uint32_t buf_size)
  \brief       Open SDS stream.
  \param[in]   name           SDS stream name (pointer to NULL terminated string)
  \param[in]   mode           SDS stream opening mode (see \ref sdsMode_t)
  \param[in]   buf            pointer to buffer for SDS stream
  \param[in]   buf_size       buffer size in bytes
  \return      \ref sdsId_t handle to SDS stream, or NULL if operation failed
*/
sdsId_t sdsOpen (const char *name, sdsMode_t mode, void *buf, uint32_t buf_size);

/**
  \fn          int32_t sdsClose (sdsId_t id)
  \brief       Close SDS stream.
  \param[in]   id             \ref sdsId_t handle to SDS stream
  \return      SDS_OK on success or
               a negative value on error (see \ref SDS_Return_Codes)
*/
int32_t sdsClose (sdsId_t id);

/**
  \fn          int32_t sdsWrite (sdsId_t id, uint32_t timeslot, const void *buf, uint32_t buf_size)
  \brief       Write entire data block along with its time slot information to the SDS stream opened in write mode.
  \param[in]   id             \ref sdsId_t handle to SDS stream
  \param[in]   timeslot       time slot
  \param[in]   buf            pointer to the data block buffer to be written
  \param[in]   buf_size       size of the data block buffer in bytes
  \return      number of bytes successfully written or
               a negative value on error (see \ref SDS_Return_Codes)
*/
int32_t sdsWrite (sdsId_t id, uint32_t timeslot, const void *buf, uint32_t buf_size);

/**
  \fn          int32_t sdsRead (sdsId_t id, uint32_t *timeslot, void *buf, uint32_t buf_size)
  \brief       Read entire data block along with its time slot information from the SDS stream opened in read mode.
  \param[in]   id             \ref sdsId_t handle to SDS stream
  \param[out]  timeslot       pointer to buffer for a time slot value
  \param[out]  buf            pointer to the data block buffer to be read
  \param[in]   buf_size       size of the data block buffer in bytes
  \return      number of bytes successfully read, or
               a negative value on error or SDS_EOS (see \ref SDS_Return_Codes)
*/
int32_t sdsRead (sdsId_t id, uint32_t *timeslot, void *buf, uint32_t buf_size);

/**
  \fn          int32_t sdsGetSize (sdsId_t id)
  \brief       Get data block size from an SDS stream opened in read mode.
  \param[in]   id             \ref sdsId_t handle to stream
  \return      number of bytes in next available data block, or
               a negative value on error or SDS_EOS (see \ref SDS_Return_Codes)
*/
int32_t sdsGetSize (sdsId_t id);


// ==== SDS Control Interface ====

// Error information structure
typedef struct {                    
  int32_t status;                       // Error status code (see \ref SDS_Return_Codes)
  const char *file;                     // File name where error occurred
  uint32_t line;                        // Line number where error occurred
  uint8_t occurred;                     // Flag indicating that an error has occurred
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
  }

#endif

#ifndef SDS_PRINTF

// Print messages to STDIO
#define SDS_PRINTF(...)                                         \
  printf(__VA_ARGS__)

#endif

// sdsFlags bitmask definitions
#define SDS_FLAG_START     (1U << 31)   // sdsFlags.31 = 1 for start of the SDS recording/playback
#define SDS_FLAG_TERMINATE (1U << 30)   // sdsFlags.30 = 1 for terminating CI run (on FVP simulation or pyOCD)
#define SDS_FLAG_PLAYBACK  (1U << 29)   // sdsFlags.29 = 1 for playback mode
#define SDS_FLAG_ALIVE     (1U << 28)   // sdsFlags.28 = 1 Host is alive
#define SDS_FLAG_RESET     (1U << 27)   // sdsFlags.27 = 1 for resetting application
                                        // sdsFlags.24..26 reserved for future enhancements
                                        // sdsFlags.0..23 for user options (i.e. bypassing filter, etc.)

// Global configuration options and diagnostic storage information (flags)
extern volatile uint32_t sdsFlags;      // SDS control flags (see \ref SDS_Flags)

// sdsState value definitions
#define SDS_STATE_INACTIVE          0   // Streaming is not active and Device is not connected to Host
#define SDS_STATE_CONNECTED         1   // Device is connected to Host
#define SDS_STATE_START             2   // Request to start streaming, open streams and get ready for read/write operations
#define SDS_STATE_ACTIVE            3   // Streaming is active
#define SDS_STATE_STOP_REQ          4   // Request to stop streaming and close streams
#define SDS_STATE_STOP_DONE         5   // Streaming stopped
#define SDS_STATE_END               6   // Request to end streaming (no more data)
#define SDS_STATE_RESET             7   // Request to reset application
#define SDS_STATE_TERMINATE         8   // Request to terminate application

// Global state information
extern volatile uint32_t sdsState;      // SDS states (see \ref SDS_States)

// Global idle rate information
extern volatile uint32_t sdsIdleRate;

/**
  \fn          int32_t sdsExchange (void)
  \brief       Exchange information with the Host.
               Update sdsFlags if requested by the Host, and send current sdsFlags
               value along with sdsIdleRate and optional error information (sdsError) to the Host.
  \return      SDS_OK on success or
               a negative value on error (see \ref SDS_Return_Codes)
*/
int32_t sdsExchange (void);

/**
  \fn          void sdsFlagsModify (uint32_t set_mask, uint32_t clear_mask)
  \brief       Modify SDS control flags (atomic operation).
  \param[in]   set_mask        bits to set in sdsFlags
  \param[in]   clear_mask      bits to clear in sdsFlags
*/
void sdsFlagsModify (uint32_t set_mask, uint32_t clear_mask);

#ifdef  __cplusplus
}
#endif

#endif  /* SDS_H */
