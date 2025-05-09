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

/**
  \typedef void (*sdsRecPlayEvent_t) (sdsRecPlayId_t id, uint32_t event)
  \brief       Callback function for recorder and player events
  \param[in]   id             \ref sdsRecPlayId_t handle to SDS Recorder/Player stream
  \param[in]   event          event code (see \ref SDS_Recorder_Player_Event_Codes)
*/
typedef void (*sdsRecPlayEvent_t) (sdsRecPlayId_t id, uint32_t event);

/**
  \fn          int32_t sdsRecPlayInit (sdsRecPlayEvent_t event_cb)
  \brief       Initialize recorder and player.
  \param[in]   event_cb       pointer to \ref sdsRecPlayEvent_t callback function
  \return      SDS_REC_PLAY_OK on success or
               a negative value on error (see \ref SDS_Recorder_Player_Return_Codes)
*/
int32_t sdsRecPlayInit (sdsRecPlayEvent_t event_cb);

/**
  \fn          int32_t sdsRecPlayUninit (void)
  \brief       Uninitialize recorder and player.
  \return      SDS_REC_PLAY_OK on success or
               a negative value on error (see \ref SDS_Recorder_Player_Return_Codes)
*/
int32_t sdsRecPlayUninit (void);

/**
  \fn          sdsRecPlayId_t sdsRecOpen (const char *name, void *buf, uint32_t buf_size)
  \brief       Open recorder stream (write mode).
  \param[in]   name           stream name (pointer to NULL terminated string)
  \param[in]   buf            pointer to buffer for recorder stream
  \param[in]   buf_size       buffer size in bytes
  \return      \ref sdsRecPlayId_t handle to SDS Recorder/Player stream, or NULL if operation failed
*/
sdsRecPlayId_t sdsRecOpen (const char *name, void *buf, uint32_t buf_size);

/**
  \fn          int32_t sdsRecClose (sdsRecPlayId_t id)
  \brief       Close recorder stream.
  \param[in]   id             \ref sdsRecPlayId_t handle to SDS Recorder/Player stream
  \return      SDS_REC_PLAY_OK on success or
               a negative value on error (see \ref SDS_Recorder_Player_Return_Codes)
*/
int32_t sdsRecClose (sdsRecPlayId_t id);

/**
  \fn          int32_t sdsRecWrite (sdsRecPlayId_t id, uint32_t timestamp, const void *buf, uint32_t buf_size)
  \brief       Write entire data block along with its timestamp to the recorder stream.
  \param[in]   id             \ref sdsRecPlayId_t handle to SDS Recorder/Player stream
  \param[in]   timestamp      timestamp in ticks
  \param[in]   buf            pointer to the data block buffer to be written
  \param[in]   buf_size       size of the data block buffer in bytes
  \return      number of bytes successfully written or
               a negative value on error (see \ref SDS_Recorder_Player_Return_Codes)
*/
int32_t sdsRecWrite (sdsRecPlayId_t id, uint32_t timestamp, const void *buf, uint32_t buf_size);

/**
  \fn          sdsRecPlayId_t sdsPlayOpen (const char *name, void *buf, uint32_t buf_size)
  \brief       Open player stream (read mode).
  \param[in]   name           stream name (pointer to NULL terminated string)
  \param[in]   buf            pointer to buffer for player stream
  \param[in]   buf_size       buffer size in bytes
  \return      \ref sdsRecPlayId_t handle to SDS Recorder/Player stream, or NULL if operation failed
*/
sdsRecPlayId_t sdsPlayOpen (const char *name, void *buf, uint32_t buf_size);

/**
  \fn          int32_t sdsPlayClose (sdsRecPlayId_t id)
  \brief       Close player stream.
  \param[in]   id             \ref sdsRecPlayId_t handle to SDS Recorder/Player stream
  \return      SDS_REC_PLAY_OK on success or
               a negative value on error (see \ref SDS_Recorder_Player_Return_Codes)
*/
int32_t sdsPlayClose (sdsRecPlayId_t id);

/**
  \fn          int32_t sdsPlayRead (sdsRecPlayId_t id, uint32_t *timestamp, void *buf, uint32_t buf_size)
  \brief       Read entire data block along with its timestamp from the player stream.
  \param[in]   id             \ref sdsRecPlayId_t handle to SDS Recorder/Player stream
  \param[out]  timestamp      pointer to buffer for a timestamp in ticks
  \param[out]  buf            pointer to the data block buffer to be read
  \param[in]   buf_size       size of the data block buffer in bytes
  \return      number of bytes successfully read, or
               a negative value on error or EOS (see \ref SDS_Recorder_Player_Return_Codes)
*/
int32_t sdsPlayRead (sdsRecPlayId_t id, uint32_t *timestamp, void *buf, uint32_t buf_size);

/**
  \fn          int32_t sdsPlayGetSize (sdsRecPlayId_t id)
  \brief       Get data block size from Player stream.
  \param[in]   id             \ref sdsRecPlayId_t handle to SDS Recorder/Player stream
  \return      number of bytes in next available data block, or
               a negative value on error or EOS (see \ref SDS_Recorder_Player_Return_Codes)
*/
int32_t sdsPlayGetSize (sdsRecPlayId_t id);

#ifdef  __cplusplus
}
#endif

#endif  /* SDS_REC_PLAY_H */
