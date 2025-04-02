/*
 * Copyright (c) 2022-2025 Arm Limited. All rights reserved.
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

#ifndef SDS_BUFFER_H
#define SDS_BUFFER_H

#ifdef  __cplusplus
extern "C"
{
#endif

#include <stdint.h>

// ==== Synchronous Data Stream (SDS) Buffer ====

typedef void *sdsBufferId_t;                        ///< Handle to SDS buffer stream

/// Function return codes
#define SDS_BUFFER_OK                   (0)         ///< Operation completed successfully
#define SDS_BUFFER_ERROR                (-1)        ///< Operation failed

/// Events
#define SDS_BUFFER_EVENT_DATA_LOW       (1UL << 0)  ///< Data bellow or equal to threshold
#define SDS_BUFFER_EVENT_DATA_HIGH      (1UL << 1)  ///< Data above or equal to threshold

/**
  \typedef void (*sdsBufferEvent_t) (sdsBufferId_t id, uint32_t event, void *arg)
  \brief       Call back function for SDS circular buffer handling
  \param[in]   id             \ref sdsBufferId_t handle to SDS buffer stream
  \param[in]   event          event code
  \param[in]   arg            pointer to argument
*/
typedef void (*sdsBufferEvent_t) (sdsBufferId_t id, uint32_t event, void *arg);

/**
  \fn          sdsBufferId_t sdsBufferOpen (void *buf, uint32_t buf_size, uint32_t threshold_low, uint32_t threshold_high)
  \brief       Open SDS buffer stream.
  \param[in]   buf            pointer to buffer for stream
  \param[in]   buf_size       buffer size in bytes
  \param[in]   threshold_low  data low threshold in bytes
  \param[in]   threshold_high data high threshold in bytes
  \return      \ref sdsId_t   Handle to SDS buffer stream
*/
sdsBufferId_t sdsBufferOpen (void *buf, uint32_t buf_size, uint32_t threshold_low, uint32_t threshold_high);

/**
  \fn          int32_t sdsBufferClose (sdsBufferId_t id)
  \brief       Close SDS buffer stream.
  \param[in]   id             \ref sdsBufferId_t handle to SDS buffer stream
  \return      return code
*/
int32_t sdsBufferClose (sdsBufferId_t id);

/**
  \fn          int32_t sdsBufferRegisterEvents (sdsBufferId_t id, sdsBufferEvent_t event_cb, uint32_t event_mask, void *event_arg)
  \brief       Register SDS buffer stream events.
  \param[in]   id             \ref sdsBufferId_t handle to SDS buffer stream
  \param[in]   event_cb       pointer to \ref sdsEvent_t
  \param[in]   event_mask     event mask
  \param[in]   event_arg      event argument
  \return      return code
*/
int32_t sdsBufferRegisterEvents (sdsBufferId_t id, sdsBufferEvent_t event_cb, uint32_t event_mask, void *event_arg);

/**
  \fn          uint32_t sdsBufferWrite (sdsBufferId_t id, const void *buf, uint32_t buf_size)
  \brief       Write data to SDS buffer stream.
  \param[in]   id             \ref sdsBufferId_t handle to SDS buffer stream
  \param[in]   buf            pointer to buffer with data to write
  \param[in]   buf_size       buffer size in bytes
  \return      number of bytes written
*/
uint32_t sdsBufferWrite (sdsBufferId_t id, const void *buf, uint32_t buf_size);

/**
  \fn          uint32_t sdsBufferRead (sdsBufferId_t id, void *buf, uint32_t buf_size)
  \brief       Read data from SDS buffer stream.
  \param[in]   id             \ref sdsBufferId_t handle to SDS buffer stream
  \param[out]  buf            pointer to buffer for data to read
  \param[in]   buf_size       buffer size in bytes
  \return      number of bytes read
*/
uint32_t sdsBufferRead (sdsBufferId_t id, void *buf, uint32_t buf_size);

/**
  \fn          int32_t sdsBufferClear (sdsBufferId_t id)
  \brief       Clear SDS buffer stream data.
  \param[in]   id             \ref sdsBufferId_t handle to SDS buffer stream
  \return      return code
*/
int32_t sdsBufferClear (sdsBufferId_t id);

/**
  \fn          uint32_t sdsBufferGetCount (sdsBufferId_t id)
  \brief       Get data count in SDS buffer stream.
  \param[in]   id             \ref sdsBufferId_t handle to SDS buffer stream
  \return      number of bytes in buffer stream
*/
uint32_t sdsBufferGetCount (sdsBufferId_t id);

#ifdef  __cplusplus
}
#endif

#endif  /* SDS_BUFFER_H */
