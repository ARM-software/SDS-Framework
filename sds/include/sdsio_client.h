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

#ifndef SDSIO_CLIENT_H
#define SDSIO_CLIENT_H

#ifdef  __cplusplus
 extern "C"
{
#endif

// SDSIO Client works in a pair with SDSIO Server. Communication protocol is documented in the following link:
// https://arm-software.github.io/SDS-Framework/main/theory/#sdsio-server-protocol

 // SDSIO header
typedef struct {
  uint32_t command;
  uint32_t sdsio_id;
  uint32_t argument;
  uint32_t data_size;
} header_t;

// SDSIO Server Command IDs
#define SDSIO_CMD_OPEN          1U
#define SDSIO_CMD_CLOSE         2U
#define SDSIO_CMD_WRITE         3U
#define SDSIO_CMD_READ          4U
#define SDSIO_CMD_EOS           5U
#define SDSIO_CMD_PING          6U

// Function prototypes

/**
  \fn          int32_t sdsioClientInit (void)
  \brief       Initialize SDSIO Client and ping SDSIO Server to verify connection
  \return      SDSIO_OK: initialization success
               SDSIO_ERROR: initialization or ping server failed
*/
int32_t sdsioClientInit (void);

/**
  \fn          int32_t sdsioClientUninit (void)
  \brief       Un-Initialize SDSIO Client
  \return      SDIOS_OK: un-initialization success
               SDSIO_ERROR: un-initialization failed
*/
int32_t sdsioClientUninit (void);

/**
  \fn          uint32_t sdsioClientSend (const header_t *header, const void *data, uint32_t data_size)
  \brief       Send data to SDSIO-Server
  \param[in]   header       pointer to header
  \param[in]   data         pointer to buffer with data to send
  \param[in]   data_size    data size in bytes
  \return      number of bytes sent (including header)
*/
uint32_t sdsioClientSend (const header_t *header, const void *data, uint32_t data_size);

/**
  \fn          uint32_t sdsioClientReceive (header_t *header, void *data, uint32_t data_size)
  \brief       Receive data from SDSIO-Server
  \param[out]  header       pointer to header
  \param[out]  data         pointer to buffer for data to read
  \param[in]   data_size    data size in bytes
  \return      number of bytes received (including header)
*/
uint32_t sdsioClientReceive (header_t *header, void *data, uint32_t data_size);

#ifdef  __cplusplus
}
#endif

#endif  /* SDSIO_CLIENT_H */