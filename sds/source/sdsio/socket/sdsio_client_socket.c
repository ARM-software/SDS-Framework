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
  \return      SDSIO_OK on success or
               a negative value on error (see \ref SDS_IO_Return_Codes)
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
  \return      SDSIO_OK on success or
               a negative value on error (see \ref SDS_IO_Return_Codes)
*/
int32_t sdsioClientUninit (void) {
  iotSocketClose(socket);
  socket = -1;
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
  int32_t num = 0U;
  int32_t ret = SDSIO_ERROR;
  int32_t sock_status;

  while (num < buf_size) {
    sock_status = iotSocketSend(socket, buf + num, buf_size - num);
    if (sock_status >= 0) {
      num += sock_status;
    } else {
      if (sock_status == IOT_SOCKET_EAGAIN) {
        // Timeout happened.
        ret = SDSIO_ERROR_TIMEOUT;
      } else {
        // Error happened.
        ret = SDSIO_ERROR_INTERFACE;
      }
      break;
    }
  }
  if (num != 0U) {
    ret = num;
  }
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
  int32_t num = 0U;
  int32_t ret = SDSIO_ERROR;
  int32_t sock_status;

  while (num < buf_size) {
    sock_status = iotSocketRecv(socket, buf + num, buf_size - num);
    if (sock_status >= 0) {
      num += sock_status;
    } else {
      if (sock_status == IOT_SOCKET_EAGAIN) {
        // Timeout happened.
        ret = SDSIO_ERROR_TIMEOUT;
      } else {
        // Error happened.
        ret = SDSIO_ERROR_INTERFACE;
      }
      break;
    }
  }
  if (num != 0U) {
    ret = num;
  }
  return ret;
}
