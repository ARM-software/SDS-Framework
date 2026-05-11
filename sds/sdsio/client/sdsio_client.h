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

#ifndef SDSIO_CLIENT_H
#define SDSIO_CLIENT_H

#ifdef  __cplusplus
extern "C"
{
#endif

#include <stdint.h>

// Receive mode
typedef enum {
  sdsioReceiveBlocking    = 0U,         // Receive in blocking mode
  sdsioReceiveNonBlocking = 1U          // Receive in non-blocking mode
} sdsioReceiveMode_t;

// SDSIO Client works in a pair with SDSIO Server. Communication protocol is documented in the following link:
// https://arm-software.github.io/SDS-Framework/main/theory/#sdsio-server-protocol

// Function prototypes

/**
  \fn          int32_t sdsioClientInit (void)
  \brief       Initialize SDSIO Client.
  \return      SDS_OK on success or
               a negative value on error (see \ref SDS_Return_Codes)
*/
int32_t sdsioClientInit (void);

/**
  \fn          int32_t sdsioClientUninit (void)
  \brief       Un-Initialize SDSIO Client.
  \return      SDS_OK on success or
               a negative value on error (see \ref SDS_Return_Codes)
*/
int32_t sdsioClientUninit (void);

/**
  \fn          int32_t sdsioClientSend (const uint8_t *buf, uint32_t buf_size)
  \brief       Send data to SDSIO-Server (blocking).
  \param[in]   buf         pointer to buffer with data to send
  \param[in]   buf_size    buffer size in bytes
  \return      number of bytes successfully sent or
               a negative value on error (see \ref SDS_Return_Codes)
*/
int32_t sdsioClientSend (const uint8_t *buf, uint32_t buf_size);

/**
  \fn          int32_t sdsioClientReceive (uint8_t *buf, uint32_t buf_size, sdsioReceiveMode_t mode)
  \brief       Receive data from SDSIO-Server in blocking or non-blocking mode.
  \param[out]  buf          pointer to the buffer where received data will be stored
  \param[in]   buf_size     buffer size in bytes
  \param[in]   mode         blocking or non-blocking mode (see \ref sdsioReceiveMode_t)
  \return      number of bytes successfully received or
               a negative value on error (see \ref SDS_Return_Codes)
*/
int32_t sdsioClientReceive (uint8_t *buf, uint32_t buf_size, sdsioReceiveMode_t mode);

#ifdef  __cplusplus
}
#endif

#endif  /* SDSIO_CLIENT_H */
