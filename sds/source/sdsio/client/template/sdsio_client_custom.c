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

// SDS I/O Client: Template for custom implementation

#include "sds.h"
#include "sdsio.h"
#include "sdsio_client.h"

/**
  \fn          int32_t sdsioClientInit (void)
  \brief       Initialize SDSIO Client I/O.
  \return      SDS_OK on success or
               a negative value on error (see \ref SDS_IO_Return_Codes)
*/
int32_t sdsioClientInit (void) {
  int32_t ret = SDS_ERROR_IO;

  // ToDo: Add code for SDS I/O Client initialization

  if (ret == SDS_OK) {
    SDS_PRINTF("SDS I/O Custom interface initialized successfully\n");
  } else {
    SDS_PRINTF("SDS I/O Custom interface initialization failed!\n");
    SDS_PRINTF("Ensure that device is connected via Custom interface to the host PC running SDSIO-Server!\n");
  }

  return ret;
}

/**
  \fn          int32_t sdsioClientUninit (void)
  \brief       Un-Initialize SDSIO Client I/O.
  \return      SDS_OK on success or
               a negative value on error (see \ref SDS_IO_Return_Codes)
*/
int32_t sdsioClientUninit (void) {

  // ToDo: Add code for SDS I/O Client de-initialization

  return SDS_OK;
}

/**
  \fn          int32_t sdsioClientSend (const uint8_t *buf, uint32_t buf_size)
  \brief       Send data to SDSIO-Server (blocking).
  \param[in]   buf         pointer to buffer with data to send
  \param[in]   buf_size    buffer size in bytes
  \return      number of bytes successfully sent or
               a negative value on error (see \ref SDS_IO_Return_Codes)
*/
int32_t sdsioClientSend (const uint8_t *buf, uint32_t buf_size) {
  int32_t ret = SDS_ERROR_IO;

  // ToDo: Add code for sending data in buf to SDSIO-Server in blocking mode

  return ret;
}

/**
  \fn          int32_t sdsioClientReceive (uint8_t *buf, uint32_t buf_size, sdsioReceiveMode_t mode)
  \brief       Receive data from SDSIO-Server in blocking or non-blocking mode.
  \param[out]  buf          pointer to the buffer where received data will be stored
  \param[in]   buf_size     buffer size in bytes
  \param[in]   mode         blocking or non-blocking mode (see \ref sdsioReceiveMode_t)
  \return      number of bytes successfully received or
               a negative value on error (see \ref SDS_IO_Return_Codes)
*/
int32_t sdsioClientReceive (uint8_t *buf, uint32_t buf_size, sdsioReceiveMode_t mode) {
  int32_t ret = SDS_ERROR_IO;

  // ToDo: Add code for receiving data to buf from SDSIO-Server in blocking or non-blocking mode

  return ret;
}
