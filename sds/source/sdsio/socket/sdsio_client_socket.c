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
 * distributed under the License is distributed on an AS IS BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// SDS I/O Client via Socket (IoT Utility:Socket)
#include <stdlib.h>

#include "cmsis_os2.h"
#include "cmsis_compiler.h"
#include "iot_socket.h"

#include "sdsio.h"
#include "sdsio_client.h"
#include "sdsio_config_socket.h"

static int32_t socket = -1;

// Socket startup function must be provided by a user application.
// Typically it is part of IoT Socket layer.
extern int32_t socket_startup (void);

// Retrieve the socket address from the configuration
static int32_t sdsioSocketGetIP(uint8_t *ip_buf, uint32_t buf_size) {
  int32_t i;
  char   *p, *end;

  if ((ip_buf == NULL) || (buf_size < 4)) {
    return -1;
  }

  p = SDSIO_SOCKET_SERVER_IP;
  for (i = 0; i < 4; i++, p = end + 1) {
    ip_buf[i] = (uint8_t)strtoul(p, &end, 10);
    if (i < 3 && *end != '.') {
      break;
    }
  }
  if (i != 4) {
    return -1;
  }
  return 0;
}

/**
  \fn          int32_t sdsioClientInit (void)
  \brief       Initialize SDS I/O Client via IoT Socket
  \return      SDIOS_OK: initialization success
               SDSIO_ERROR: initialization failed
*/
int32_t sdsioClientInit (void) {
  int32_t  ret = SDSIO_ERROR;
  int32_t  err = 0;
  uint32_t opt_val;
  uint8_t  ip[4];

  // Check if client is initialized
  if (socket == -1) {

    // Socket startup function must be provided by a user application.
    // Typically it is part of IoT Socket layer.
    err = socket_startup();

    // Get socket address
    if (err == 0) {
      err = sdsioSocketGetIP(ip, sizeof(ip));
    }

    if (err == 0) {
      // Create socket
      socket = iotSocketCreate(IOT_SOCKET_AF_INET, IOT_SOCKET_SOCK_STREAM, IOT_SOCKET_IPPROTO_TCP);
    }
    if (socket >= 0) {
      opt_val = SDSIO_SOCKET_RECEIVE_TOUT;
      iotSocketSetOpt(socket, IOT_SOCKET_SO_RCVTIMEO, &opt_val, sizeof(opt_val));
      opt_val = 1;
      iotSocketSetOpt(socket, IOT_SOCKET_SO_KEEPALIVE, &opt_val, sizeof(opt_val));

      if (iotSocketConnect(socket, (const uint8_t *)ip, 4, SDSIO_SOCKET_SERVER_PORT) != 0) {
        iotSocketClose(socket);
        socket = -1;
        err = -1;
      }
    }
  }

  if ((err == 0) && (socket >= 0)) {
    ret = SDSIO_OK;
  } else {
    ret = SDSIO_ERROR;
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
      cnt = 0U;
      break;
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
        cnt = 0U;
        break;
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
      cnt = 0U;
      break;
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
        cnt = 0U;
        break;
      }
    }
  }

  num += cnt;

  return num;
}
