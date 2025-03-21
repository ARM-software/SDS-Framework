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
  \return      SDIOS_OK: initialization success
               SDSIO_ERROR: initialization failed
*/
int32_t sdsioClientInit (void) {
  int32_t ret = SDSIO_ERROR;

  // ToDo: Add code for SDS I/O Client initialization

  return ret;
}

/**
  \fn          int32_t sdsioClientUninit (void)
  \brief       Un-Initialize SDS I/O Client
  \return      SDIOS_OK: un-initialization success
               SDSIO_ERROR: un-initialization failed
*/
int32_t sdsioClientUninit (void) {

  // ToDo: Add code for SDS I/O Client de-initialization

  return SDSIO_OK;
}

/**
  \fn          uint32_t sdsioClientSend (const header_t *header, const void *data, uint32_t data_size)
  \brief       Send data to SDSIO-Server
  \param[in]   header       pointer to header
  \param[in]   data         pointer to buffer with data to send
  \param[in]   data_size    data size in bytes
  \return      number of bytes sent (including header)
*/
uint32_t sdsioClientSend (const header_t *header, const void *data, uint32_t data_size) {
  uint32_t num = 0U;
  // uint32_t cnt;

  if (header == NULL) {
    return 0U;
  }

  // Send header
  // ToDo: Modify code below to send header to SDSIO-Server
  // cnt = 0U;
  // while (cnt < sizeof(header_t)) {
  //   cnt += CustomSend((const uint8_t *)header + cnt, sizeof(header_t) - cnt);
  // }
  // num = cnt;


  // Send data
  // ToDo: Modify code below to send data to SDSIO-Server
  // cnt = 0U;
  // if ((data != NULL) && (data_size != 0U)) {
  //   while (cnt < data_size) {
  //     cnt += CustomSend((const uint8_t *)data + cnt, data_size - cnt);
  //   }
  // }
  // num += cnt;

  return num;
}

/**
  \fn          uint32_t sdsioClientReceive (header_t *header, void *data, uint32_t data_size)
  \brief       Receive data from SDSIO-Server
  \param[out]  header       pointer to header
  \param[out]  data         pointer to buffer for data to read
  \param[in]   data_size    data size in bytes
  \return      number of bytes received (including header)
*/
uint32_t sdsioClientReceive (header_t *header, void *data, uint32_t data_size) {
  uint32_t num = 0U;
  // uint32_t cnt, size;

  if (header == NULL) {
    return 0U;
  }

  // Receive header
  // ToDo: Modify code below to receive header from SDSIO-Server
  // cnt = 0U;
  // while (cnt < sizeof(header_t)) {
  //   cnt += CustomReceive((uint8_t *)header + cnt, sizeof(header_t) - cnt);
  // }
  // num = cnt;

  // Receive data
  // ToDo: Modify code below to receive data from SDSIO-Server
  // cnt = 0U;
  // if ((num != 0U) && (header->data_size != 0U) &&
  //     (data != NULL) && (data_size != 0U)) {

  //   if (header->data_size < data_size) {
  //     size = header->data_size;
  //   } else {
  //     size = data_size;
  //   }
  //   while (cnt < size) {
  //     cnt += CustomReceive((uint8_t *)data + cnt, size - cnt);
  //   }
  // }
  // num += cnt;

  return num;
}
