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

typedef void *sdsBufferId_t;                        // Handle to SDS buffer stream

// Function return codes
#define SDS_BUFFER_OK                   (0)         // Operation completed successfully
#define SDS_BUFFER_ERROR                (-1)        // Operation failed
#define SDS_BUFFER_ERROR_PARAMETER      (-2)        // Operation failed: parameter error

// Events
#define SDS_BUFFER_EVENT_DATA_LOW       (1UL)       // Data bellow or equal to low threshold
#define SDS_BUFFER_EVENT_DATA_HIGH      (2UL)       // Data above or equal to high threshold

/**
  \typedef void (*sdsBufferEvent_t) (sdsBufferId_t id, uint32_t event, void *arg)
  \brief       Callback function for SDS circular buffer event handling
  \param[in]   id             \ref sdsBufferId_t handle to SDS buffer stream
  \param[in]   event          event code (see \ref SDS_Buffer_Event_Codes)
  \param[in]   arg            pointer to argument registered with \ref sdsBufferRegisterEvents
*/
typedef void (*sdsBufferEvent_t) (sdsBufferId_t id, uint32_t event, void *arg);

/**
  \fn          sdsBufferId_t sdsBufferOpen (void *buf, uint32_t buf_size, uint32_t threshold_low, uint32_t threshold_high)
  \brief       Open SDS buffer stream.
  \param[in]   buf            pointer to buffer for stream
  \param[in]   buf_size       buffer size in bytes
  \param[in]   threshold_low  data low threshold in bytes
  \param[in]   threshold_high data high threshold in bytes
  \return      \ref sdsBufferId_t Handle to SDS buffer stream, or NULL if operation failed
*/
sdsBufferId_t sdsBufferOpen (void *buf, uint32_t buf_size, uint32_t threshold_low, uint32_t threshold_high);

/**
  \fn          int32_t sdsBufferClose (sdsBufferId_t id)
  \brief       Close SDS buffer stream.
  \param[in]   id             \ref sdsBufferId_t handle to SDS buffer stream
  \return      SDS_BUFFER_OK on success or
               a negative value on error (see \ref SDS_Buffer_Return_Codes)
*/
int32_t sdsBufferClose (sdsBufferId_t id);

/**
  \fn          int32_t sdsBufferRegisterEvents (sdsBufferId_t id, sdsBufferEvent_t event_cb, uint32_t event_mask, void *event_arg)
  \brief       Register SDS buffer stream event callback function.
  \param[in]   id             \ref sdsBufferId_t handle to SDS buffer stream
  \param[in]   event_cb       pointer to \ref sdsBufferEvent_t callback function, NULL to un-register
  \param[in]   event_mask     event mask
  \param[in]   event_arg      pointer to event argument
  \return      SDS_BUFFER_OK on success or
               a negative value on error (see \ref SDS_Buffer_Return_Codes)
*/
int32_t sdsBufferRegisterEvents (sdsBufferId_t id, sdsBufferEvent_t event_cb, uint32_t event_mask, void *event_arg);

/**
  \fn          int32_t sdsBufferWrite (sdsBufferId_t id, const void *buf, uint32_t buf_size)
  \brief       Write data to SDS buffer stream.
  \param[in]   id             \ref sdsBufferId_t handle to SDS buffer stream
  \param[in]   buf            pointer to buffer with data to write
  \param[in]   buf_size       buffer size in bytes
  \return      number of data bytes successfully written or
               a negative value on error (see \ref SDS_Buffer_Return_Codes)
*/
int32_t sdsBufferWrite (sdsBufferId_t id, const void *buf, uint32_t buf_size);

/**
  \fn          int32_t sdsBufferRead (sdsBufferId_t id, void *buf, uint32_t buf_size)
  \brief       Read data from SDS buffer stream.
  \param[in]   id             \ref sdsBufferId_t handle to SDS buffer stream
  \param[out]  buf            pointer to buffer for data to read
  \param[in]   buf_size       buffer size in bytes
  \return      number of data bytes successfully read or
               a negative value on error (see \ref SDS_IO_Return_Codes)
*/
int32_t sdsBufferRead (sdsBufferId_t id, void *buf, uint32_t buf_size);

/**
  \fn          int32_t sdsBufferClear (sdsBufferId_t id)
  \brief       Clear SDS buffer stream data.
  \param[in]   id             \ref sdsBufferId_t handle to SDS buffer stream
  \return      SDS_BUFFER_OK on success or
               a negative value on error (see \ref SDS_Buffer_Return_Codes)
*/
int32_t sdsBufferClear (sdsBufferId_t id);

/**
  \fn          int32_t sdsBufferGetCount (sdsBufferId_t id)
  \brief       Get data count in SDS buffer stream.
  \param[in]   id             \ref sdsBufferId_t handle to SDS buffer stream
  \return      number of data bytes available in buffer stream or
               a negative value on error (see \ref SDS_IO_Return_Codes)
*/
int32_t sdsBufferGetCount (sdsBufferId_t id);

#ifdef  __cplusplus
}
#endif

#endif  /* SDS_BUFFER_H */
