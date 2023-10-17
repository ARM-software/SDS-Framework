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

/// Identifier
typedef void *sdsPlayId_t;

/// Function return codes
#define SDS_PLAY_OK             (0)         ///< Operation completed successfully
#define SDS_PLAY_ERROR          (-1)        ///< Operation failed

/// Events
#define SDS_PLAY_EVENT_IO_ERROR  (1UL << 0) ///< I/O Error

/// Event callback function
typedef void (*sdsPlayEvent_t) (sdsPlayId_t id, uint32_t event);

/**
  \fn          int32_t sdsPlayInit (void)
  \brief       Initialize player.
  \param[in]   event_cb       pointer to \ref sdsPlayEvent_t
  \return      return code
*/
int32_t sdsPlayInit (sdsPlayEvent_t event_cb);

/**
  \fn          int32_t sdsPlayUninit (void)
  \brief       Uninitialize player.
  \return      return code
*/
int32_t sdsPlayUninit (void);

/**
  \fn          sdsPlayId_t sdsPlayOpen (const char *name, void *buf, uint32_t buf_size, uint32_t record_size)
  \brief       Open player stream.
  \param[in]   name           stream name (pointer to NULL terminated string)
  \param[in]   buf            pointer to buffer for stream
  \param[in]   buf_size       buffer size in bytes
  \param[in]   io_threshold   threshold in bytes to trigger I/O read (when below threshold)
  \return      \ref sdsPlayId_t
*/
sdsPlayId_t sdsPlayOpen (const char *name, void *buf, uint32_t buf_size, uint32_t io_threshold);

/**
  \fn          int32_t sdsPlayClose (sdsPlayId_t id)
  \brief       Close player stream.
  \param[in]   id             \ref sdsPlayId_t
  \return      return code
*/
int32_t sdsPlayClose (sdsPlayId_t id);

/**
  \fn          uint32_t sdsPlayRead (sdsPlayId_t id, void *buf, uint32_t buf_size)
  \brief       Read record data and timestamp from Player stream.
  \param[in]   id             \ref sdsPlayId_t
  \param[out]  timestamp      pointer to buffer for record timestamp in ticks
  \param[out]  buf            pointer to buffer for data to read
  \param[in]   buf_size       buffer size in bytes
  \return      number of data bytes read
*/
uint32_t sdsPlayRead (sdsPlayId_t id, uint32_t *timestamp, void *buf, uint32_t buf_size);

/**
  \fn          uint32_t sdsPlayGetSize (sdsPlayId_t id)
  \brief       Get record data size from Player stream.
  \param[in]   id             \ref sdsPlayId_t
  \return      number of data bytes in record
*/
uint32_t sdsPlayGetSize (sdsPlayId_t id);

/**
  \fn          int32_t sdsPlayEndOfStream (sdsPlayId_t id)
  \brief       Check if end of stream has been reached.
  \param[in]   id             \ref sdsPlayId_t
  \return      nonzero if end of stream, else 0
*/
int32_t sdsPlayEndOfStream (sdsPlayId_t id);

#ifdef  __cplusplus
}
#endif

#endif  /* SDS_PLAY_H */
