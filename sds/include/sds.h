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

/// Identifier
typedef void *sdsId_t;

/// Function return codes
#define SDS_OK                  (0)         ///< Operation completed successfully
#define SDS_ERROR               (-1)        ///< Operation failed

/// Events
#define SDS_EVENT_DATA_LOW      (1UL << 0)  ///< Data bellow or equal to threshold
#define SDS_EVENT_DATA_HIGH     (1UL << 1)  ///< Data above or equal to threshold

/// Event callback function
typedef void (*sdsEvent_t) (sdsId_t id, uint32_t event, void *arg);

/**
  \fn          sdsId_t sdsOpen (void *buf, uint32_t buf_size, uint32_t record_size)
  \brief       Open stream.
  \param[in]   buf            pointer to buffer for stream
  \param[in]   buf_size       buffer size in bytes
  \param[in]   threshold_low  data low threshold in bytes
  \param[in]   threshold_high data high threshold in bytes
  \return      \ref sdsId_t
*/
sdsId_t sdsOpen (void *buf, uint32_t buf_size, uint32_t threshold_low, uint32_t threshold_high);

/**
  \fn          int32_t sdsClose (sdsId_t id)
  \brief       Close stream.
  \param[in]   id             \ref sdsId_t
  \return      return code
*/
int32_t sdsClose (sdsId_t id);

/**
  \fn          int32_t  sdsRegisterEvents (sdsId_t id, sdsEvent_t event_cb, uint32_t event_mask, void *event_arg)
  \brief       Register stream events.
  \param[in]   id             \ref sdsId_t
  \param[in]   event_cb       pointer to \ref sdsEvent_t
  \param[in]   event_mask     event mask
  \param[in]   event_arg      event argument
  \return      return code
*/
int32_t sdsRegisterEvents (sdsId_t id, sdsEvent_t event_cb, uint32_t event_mask, void *event_arg);

/**
  \fn          uint32_t sdsWrite (sdsId_t id, const void *buf, uint32_t buf_size)
  \brief       Write data to stream.
  \param[in]   id             \ref sdsId_t
  \param[in]   buf            pointer to buffer with data to write
  \param[in]   buf_size       buffer size in bytes
  \return      number of bytes written
*/
uint32_t sdsWrite (sdsId_t id, const void *buf, uint32_t buf_size);

/**
  \fn          uint32_t sdsRead (sdsId_t id, void *buf, uint32_t buf_size)
  \brief       Read data from stream.
  \param[in]   id             \ref sdsId_t
  \param[out]  buf            pointer to buffer for data to read
  \param[in]   buf_size       buffer size in bytes
  \return      number of bytes read
*/
uint32_t sdsRead (sdsId_t id, void *buf, uint32_t buf_size);

/**
  \fn          int32_t sdsClear (sdsId_t id)
  \brief       Clear stream data.
  \param[in]   id             \ref sdsId_t
  \return      return code
*/
int32_t sdsClear (sdsId_t id);

/**
  \fn          uint32_t sdsGetCount (sdsId_t id)
  \brief       Get data count in stream.
  \param[in]   id             \ref sdsId_t
  \return      number of bytes in stream
*/
uint32_t sdsGetCount (sdsId_t id);

#ifdef  __cplusplus
}
#endif

#endif  /* SDS_H */
