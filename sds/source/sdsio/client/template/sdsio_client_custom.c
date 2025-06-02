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

// SDS I/O Client: Template for custom implementation

#include "sdsio.h"
#include "sdsio_client.h"

/**
  \fn          int32_t sdsioClientInit (void)
  \brief       Initialize SDS I/O Client
  \return      SDSIO_OK on success or
               a negative value on error (see \ref SDS_IO_Return_Codes)
*/
int32_t sdsioClientInit (void) {
  int32_t ret = SDSIO_ERROR;

  // ToDo: Add code for SDS I/O Client initialization

  return ret;
}

/**
  \fn          int32_t sdsioClientUninit (void)
  \brief       Un-Initialize SDS I/O Client
  \return      SDSIO_OK on success or
               a negative value on error (see \ref SDS_IO_Return_Codes)
*/
int32_t sdsioClientUninit (void) {

  // ToDo: Add code for SDS I/O Client de-initialization

  return SDSIO_OK;
}

/**
  \fn          int32_t sdsioClientSend (const uint8_t *buf, uint32_t buf_size)
  \brief       Send data to SDSIO-Server
  \param[in]   buf         pointer to buffer with data to send
  \param[in]   buf_size    buffer size in bytes
  \return      number of bytes successfully sent or
               a negative value on error (see \ref SDS_IO_Return_Codes)
*/
int32_t sdsioClientSend (const uint8_t *buf, uint32_t buf_size) {
  int32_t ret = SDSIO_ERROR;

  // int32_t status;

  // ToDo: Modify code below to send data in buf to SDSIO-Server
  // status = CustomSend(buf, buf_size);
  // if (status == TIMEOUT) {
  //   // Timeout happened.
  //   ret = SDSIO_ERROR_TIMEOUT;
  // } else if (status == ERROR) {
  //   // Error happened.
  //   ret = SDSIO_ERROR_INTEFACE;
  // } else {
  //   // Data sent successfully.
  //   // Return num of bytes sent.
  //   ret = status;
  // }

  return ret;
}

/**
  \fn          int32_t sdsioClientReceive (uint8_t *buf, uint32_t buf_size)
  \brief       Receive data from SDSIO-Server
  \param[out]  buf          pointer to buffer for data to read
  \param[in]   buf_size     buffer size in bytes
  \return      number of bytes successfully received or
               a negative value on error (see \ref SDS_IO_Return_Codes)
*/
int32_t sdsioClientReceive (uint8_t *buf, uint32_t buf_size) {
  int32_t ret = SDSIO_ERROR;

  // int32_t status;

  // ToDo: Modify code below to receive data from SDSIO-Server
  // status = CustomReceive(buf, buf_size);
  // if (status == TIMEOUT) {
  //   // Timeout happened.
  //   ret = SDSIO_ERROR_TIMEOUT;
  // } else if (status == ERROR) {
  //   // Error happened.
  //   ret = SDSIO_ERROR_INTEFACE;
  // } else {
  //   // Data  received successfully.
  //   // Return num of bytes received.
  //   ret = status;
  // }

  return ret;
}
