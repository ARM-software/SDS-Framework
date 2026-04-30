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
#define SDS_OK                  (0)     ///< Function completed successfully.
#define SDS_ERROR               (-1)    ///< Unspecified error.
#define SDS_ERROR_PARAMETER     (-2)    ///< Invalid parameter passed to a function.
#define SDS_ERROR_TIMEOUT       (-3)    ///< Function execution timed out.
#define SDS_ERROR_IO            (-4)    ///< I/O error during function execution.
#define SDS_NO_SPACE            (-5)    ///< Insufficient space in SDS circular buffer to write the entire data block.
#define SDS_NO_DATA             (-6)    ///< Insufficient data in SDS stream buffer.
#define SDS_EOS                 (-7)    ///< End of stream reached in read operation.

// Event codes for sdsEvent callback function
#define SDS_EVENT_ERROR_IO      (1UL)   ///< Event triggered when an SDS I/O error occurs.
#define SDS_EVENT_NO_SPACE      (2UL)   ///< Event triggered when \ref sdsWrite fails due to insufficient space in the SDS circular buffer.
#define SDS_EVENT_NO_DATA       (4UL)   ///< Event triggered when \ref sdsRead fails due to insufficient data in the SDS circular buffer.

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
  int32_t status;                       ///< The error status code (see \ref SDS_Return_Codes).
  const char *file;                     ///< Pointer to the file name in which the error occurred.
  uint32_t line;                        ///< Line number at which the error occurred.
  uint8_t occurred;                     ///< Flag indicating that an error has occurred.
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
#define SDS_FLAG_START     (0x80000000UL)   ///< Flag controlling streaming (set to start, cleared to stop).
#define SDS_FLAG_TERMINATE (0x40000000UL)   ///< Flag for terminating CI run (on FVP simulation or pyOCD).
#define SDS_FLAG_PLAYBACK  (0x20000000UL)   ///< Flag for switching between recording and playback mode (set for playback; unset for recording).
#define SDS_FLAG_ALIVE     (0x10000000UL)   ///< Flag used by host to signal it is alive (set when the host is alive; unset when the host is not alive).
#define SDS_FLAG_RESET     (0x08000000UL)   ///< Flag used to request firmware reset (set to reset the firmware).
                                            // Bits 24..26 are reserved for future enhancements
                                            // Bits 0..23 for used for user options (i.e. bypassing filter, etc.)

// Configuration options and control information
extern volatile uint32_t sdsFlags;

// sdsState value definitions
#define SDS_STATE_INACTIVE      (0UL)   ///< Device is not connected to the host and streaming is not active.
#define SDS_STATE_CONNECTED     (1UL)   ///< Device is connected to the host, but streaming is not active.
#define SDS_STATE_START         (2UL)   ///< Request to start streaming; open streams and get ready for read/write operations.
#define SDS_STATE_ACTIVE        (3UL)   ///< Streaming is active.
#define SDS_STATE_STOP_REQ      (4UL)   ///< Request to stop streaming and close all open streams.
#define SDS_STATE_STOP_DONE     (5UL)   ///< Streaming has stopped.
#define SDS_STATE_END           (6UL)   ///< Request to end streaming (e.g., no more playback data is available).
#define SDS_STATE_RESET         (7UL)   ///< Request to reset the device.
#define SDS_STATE_TERMINATE     (8UL)   ///< Request to terminate the active session.

// State information
extern volatile uint32_t sdsState;

// Idle rate information
extern volatile uint32_t sdsIdleRate;

/**
  \fn          int32_t sdsExchange (void)
  \brief       Exchange information with the host.
  \return      SDS_OK on success or
               a negative value on error (see \ref SDS_Return_Codes)
*/
int32_t sdsExchange (void);

/**
  \fn          void sdsFlagsModify (uint32_t set_mask, uint32_t clear_mask)
  \brief       Modify \ref sdsFlags control flags (atomic operation).
  \param[in]   set_mask        bits to set in sdsFlags (see \ref SDS_Flag_Masks)
  \param[in]   clear_mask      bits to clear in sdsFlags (see \ref SDS_Flag_Masks)
*/
void sdsFlagsModify (uint32_t set_mask, uint32_t clear_mask);

#ifdef  __cplusplus
}
#endif

#endif  /* SDS_H */
