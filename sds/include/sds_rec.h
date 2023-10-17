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

#ifndef SDS_REC_H
#define SDS_REC_H

#ifdef  __cplusplus
extern "C"
{
#endif

#include <stdint.h>

// ==== SDS Recorder ====

/// Identifier
typedef void *sdsRecId_t;

/// Function return codes
#define SDS_REC_OK              (0)         ///< Operation completed successfully
#define SDS_REC_ERROR           (-1)        ///< Operation failed

/// Events
#define SDS_REC_EVENT_IO_ERROR  (1UL << 0)  ///< I/O Error

/// Event callback function
typedef void (*sdsRecEvent_t) (sdsRecId_t id, uint32_t event);

/**
  \fn          int32_t sdsRecInit (sdsRecEvent_t event_cb)
  \brief       Initialize recorder.
  \param[in]   event_cb       pointer to \ref sdsRecEvent_t
  \return      return code
*/
int32_t sdsRecInit (sdsRecEvent_t event_cb);

/**
  \fn          int32_t sdsRecUninit (void)
  \brief       Uninitialize recorder.
  \return      return code
*/
int32_t sdsRecUninit (void);

/**
  \fn          sdsRecId_t sdsRecOpen (const char *name, void *buf, uint32_t buf_size, uint32_t record_size)
  \brief       Open recorder stream.
  \param[in]   name           stream name (pointer to NULL terminated string)
  \param[in]   buf            pointer to buffer for stream
  \param[in]   buf_size       buffer size in bytes
  \param[in]   io_threshold   threshold in bytes to trigger I/O write (when equal or above threshold)
  \return      \ref sdsRecId_t
*/
sdsRecId_t sdsRecOpen (const char *name, void *buf, uint32_t buf_size, uint32_t io_threshold);

/**
  \fn          int32_t sdsRecClose (sdsRecId_t id)
  \brief       Close recorder stream.
  \param[in]   id             \ref sdsRecId_t
  \return      return code
*/
int32_t sdsRecClose (sdsRecId_t id);

/**
  \fn          uint32_t sdsRecWrite (sdsRecId_t id, const void *buf, uint32_t buf_size)
  \brief       Write record data and timestamp to recorder stream.
  \param[in]   id             \ref sdsRecId_t
  \param[in]   timestamp      record timestamp in ticks
  \param[in]   buf            pointer to buffer with data to write
  \param[in]   buf_size       buffer size in bytes
  \return      number of data bytes written
*/
uint32_t sdsRecWrite (sdsRecId_t id, uint32_t timestamp, const void *buf, uint32_t buf_size);

#ifdef  __cplusplus
}
#endif

#endif  /* SDS_REC_H */
