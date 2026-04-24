/*
 * Copyright (c) 2026 Arm Limited. All rights reserved.
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

// SDS I/O Client via RTT

#include "cmsis_os2.h"
#include "cmsis_compiler.h"
#include "SEGGER_RTT.h"

#include "sds.h"
#include "sdsio_client.h"
#include "sdsio_client_rtt_config.h"

// RTT channel buffers
static uint8_t rttDownBuffer[SDSIO_RTT_DOWN_BUF_SIZE] __ALIGNED(SDSIO_RTT_BUF_ALIGN);
static uint8_t rttUpBuffer  [SDSIO_RTT_UP_BUF_SIZE]   __ALIGNED(SDSIO_RTT_BUF_ALIGN);

/**
  \fn          int32_t sdsioClientInit (void)
  \brief       Initialize SDSIO Client.
  \return      SDS_OK on success or
               a negative value on error (see \ref SDS_Return_Codes)
*/
int32_t sdsioClientInit (void) {

  SEGGER_RTT_Init();
  SEGGER_RTT_ConfigUpBuffer  (SDSIO_RTT_CHANNEL, "SDSIO_Up",   rttUpBuffer,   sizeof(rttUpBuffer),   0U);
  SEGGER_RTT_ConfigDownBuffer(SDSIO_RTT_CHANNEL, "SDSIO_Down", rttDownBuffer, sizeof(rttDownBuffer), 0U);

  return SDS_OK;
}

/**
  \fn          int32_t sdsioClientUninit (void)
  \brief       Un-Initialize SDSIO Client.
  \return      SDS_OK on success or
               a negative value on error (see \ref SDS_Return_Codes)
*/
int32_t sdsioClientUninit (void) {
  return SDS_OK;
}

/**
  \fn          int32_t sdsioClientSend (const uint8_t *buf, uint32_t buf_size)
  \brief       Send data to SDSIO-Server (blocking).
  \param[in]   buf         pointer to buffer with data to send
  \param[in]   buf_size    buffer size in bytes
  \return      number of bytes successfully sent or
               a negative value on error (see \ref SDS_Return_Codes)
*/
int32_t sdsioClientSend (const uint8_t *buf, uint32_t buf_size) {
  uint32_t num = 0U;
  uint32_t res;
  uint32_t tick;
  int32_t  ret;

  tick = osKernelGetTickCount();
  do {
    res = SEGGER_RTT_Write(SDSIO_RTT_CHANNEL, buf + num, buf_size - num);
    num += res;
    if (num >= buf_size) {
      break;
    } 
  } while ((osKernelGetTickCount() - tick) < SDSIO_RTT_TIMEOUT);

  if (num < buf_size) {
    ret = SDS_ERROR_TIMEOUT;
  } else {
    ret = (int32_t)num;
  }

  return ret;
}

/**
  \fn          int32_t sdsioClientReceive (uint8_t *buf, uint32_t buf_size, sdsioReceiveMode_t mode)
  \brief       Receive data from SDSIO-Server in blocking or non-blocking mode.
  \param[out]  buf          pointer to the buffer where received data will be stored
  \param[in]   buf_size     buffer size in bytes
  \param[in]   mode         blocking or non-blocking mode (see \ref sdsioReceiveMode_t)
  \return      number of bytes successfully received or
               a negative value on error (see \ref SDS_Return_Codes)
*/
int32_t sdsioClientReceive (uint8_t *buf, uint32_t buf_size, sdsioReceiveMode_t mode) {
  uint32_t num = 0U;
  uint32_t res;
  uint32_t tick;
  int32_t  ret;

  if (mode == sdsioReceiveNonBlocking) {
    res = SEGGER_RTT_Read(SDSIO_RTT_CHANNEL, buf, buf_size);
    ret = (int32_t)res;
    return ret;
  }

  tick = osKernelGetTickCount();
  do {
    res = SEGGER_RTT_Read(SDSIO_RTT_CHANNEL, buf + num, buf_size - num);
    num += res;
    if (num >= buf_size) {
      break;
    } 
  } while ((osKernelGetTickCount() - tick) < SDSIO_RTT_TIMEOUT);

  if (num < buf_size) {
    ret = SDS_ERROR_TIMEOUT;
  } else {
    ret = (int32_t)num;
  }

  return ret;
}
