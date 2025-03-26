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

// SDS I/O Client via Socket (IoT Utility:Socket)
#include <stdio.h>

#include "cmsis_os2.h"

#include "iot_socket.h"

#include "sdsio.h"
#include "sdsio_client.h"
#include "sdsio_config_socket.h"

static int32_t socket = -1;

/**
  \fn          int32_t sdsioClientInit (void)
  \brief       Initialize SDS I/O Client via IoT Socket
  \return      SDIOS_OK: initialization success
               SDSIO_ERROR: initialization failed
*/
int32_t sdsioClientInit (void) {
  int32_t  ret  = SDSIO_ERROR;
  uint32_t tout = SDSIO_SOCKET_RECEIVE_TOUT;
  uint8_t  ip[4];

  if (socket == -1) {
    if (sscanf(SDSIO_SERVER_IP, "%hhd.%hhd.%hhd.%hhd", &ip[0], &ip[1], &ip[2], &ip[3]) == 4) {
      socket = iotSocketCreate(IOT_SOCKET_AF_INET, IOT_SOCKET_SOCK_STREAM, IOT_SOCKET_IPPROTO_TCP);
    }
    if (socket >= 0) {
      iotSocketSetOpt(socket, IOT_SOCKET_SO_RCVTIMEO, &tout, sizeof(tout));
      if (iotSocketConnect(socket, (const uint8_t *)ip, 4, SDSIO_SERVER_PORT) == 0) {
        ret = SDSIO_OK;
      } else {
        iotSocketClose(socket);
        socket = -1;
      }
    }
  } else {
    // sdsio is already initialized
    ret = SDSIO_OK;
  }

  return ret;
}

/**
  \fn          int32_t sdsioClientUninit (void)
  \brief       Un-Initialize SDS I/O Client
  \return      SDIOS_OK: un-initialization success
               SDSIO_ERROR: un-initialization failed
*/
int32_t sdsioClientUninit (void) {
  iotSocketClose(socket);
  socket = -1;
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
  uint32_t cnt, num;
  int32_t  status;

  if (header == NULL) {
    return 0U;
  }

  // Send header
  cnt = 0U;
  while (cnt < sizeof(header_t)) {
    status = iotSocketSend(socket,
                           (const uint8_t *)header + cnt,
                           (sizeof(header_t) - cnt));
    if (status >= 0) {
      cnt += (uint32_t)status;
    } else {
      // Error
      if (status != IOT_SOCKET_EAGAIN) {
        cnt = 0U;
        break;
      }
    }
  }

  // Send data
  num = cnt;
  cnt = 0U;
  if ((num != 0U) && (data != NULL) && (data_size != 0U)) {
    while (cnt < data_size) {
      status = iotSocketSend(socket,
                             (const uint8_t *)data + cnt,
                             (data_size - cnt));
      if (status >= 0) {
        cnt += (uint32_t)status;
      } else {
        // Error
        if (status != IOT_SOCKET_EAGAIN) {
          cnt = 0U;
          break;
        }
      }
    }
  }

  num += cnt;

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
  uint32_t cnt, num, size;
  int32_t  status;

  if (header == NULL) {
    return 0U;
  }

  // Receive header
  cnt = 0U;
  while (cnt < sizeof(header_t)) {
    status = iotSocketRecv(socket,
                           (uint8_t *)header + cnt,
                           (sizeof(header_t) - cnt));
    if (status >= 0) {
      cnt += (uint32_t)status;
    } else {
      // Error
      if (status != IOT_SOCKET_EAGAIN) {
        cnt = 0U;
        break;
      }
    }
  }

  // Receive data
  num = cnt;
  cnt = 0U;
  if ((num != 0U) && (header->data_size != 0U) &&
      (data != NULL) && (data_size != 0U)) {

    if (header->data_size < data_size) {
      size = header->data_size;
    } else {
      size = data_size;
    }
    while (cnt < size) {
      status = iotSocketRecv(socket,
                             (uint8_t *)data + cnt,
                             (size - cnt));
      if (status >= 0) {
        cnt += (uint32_t)status;
      } else {
        // Error
        if (status != IOT_SOCKET_EAGAIN) {
          cnt = 0U;
          break;
        }
      }
    }
  }

  num += cnt;

  return num;
}
