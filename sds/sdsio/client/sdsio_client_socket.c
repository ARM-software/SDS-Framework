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

// SDS I/O Client via Socket (IoT Utility:Socket)
#include <stdlib.h>
#include <string.h>

#include "cmsis_os2.h"
#include "cmsis_compiler.h"
#include "iot_socket.h"

#include "sds.h"
#include "sdsio_client.h"
#include "sdsio_client_socket_config.h"

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
  \brief       Initialize SDSIO Client.
  \return      SDS_OK on success or
               a negative value on error (see \ref SDS_Return_Codes)
*/
int32_t sdsioClientInit (void) {
  int32_t  ret = SDS_ERROR_IO;
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
      opt_val = SDSIO_SOCKET_RECEIVE_TIMEOUT;
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
    SDS_PRINTF("SDS I/O socket interface initialized successfully.\n")
    SDS_PRINTF("Connection to SDSIO-Server established at %s:%d\n", SDSIO_SOCKET_SERVER_IP, SDSIO_SOCKET_SERVER_PORT);
    ret = SDS_OK;
  } else {
    if (strcmp(SDSIO_SOCKET_SERVER_IP, "0.0.0.0") == 0) {
      SDS_PRINTF("SDSIO_SOCKET_SERVER_IP address not configured (see sdsio_config_socket.h)!\n");
    } else {
      SDS_PRINTF("SDS I/O Network interface initialization failed or 'sdsio-server socket' unavailable at %s:%d !\n", SDSIO_SOCKET_SERVER_IP, SDSIO_SOCKET_SERVER_PORT);
      SDS_PRINTF("Ensure that SDSIO-Server is running, then restart the application!\n");
    }
    ret = SDS_ERROR_IO;
  }

  return ret;
}

/**
  \fn          int32_t sdsioClientUninit (void)
  \brief       Un-Initialize SDSIO Client.
  \return      SDS_OK on success or
               a negative value on error (see \ref SDS_Return_Codes)
*/
int32_t sdsioClientUninit (void) {
  if (socket != -1) {
    iotSocketClose(socket);
    socket = -1;
  }
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
  int32_t num = 0U;
  int32_t ret = SDS_ERROR_IO;
  int32_t sock_status;
  uint32_t retry = 0U;

  while (num < buf_size) {
    sock_status = iotSocketSend(socket, buf + num, buf_size - num);
    if (sock_status >= 0) {
      num += sock_status;
      retry = 0U;
    } else if ((sock_status == IOT_SOCKET_ENOMEM) && (retry < 5)) {
      osDelay(retry + 2U);
      retry++;
    } else {
      if (sock_status == IOT_SOCKET_EAGAIN) {
        // Timeout happened.
        ret = SDS_ERROR_TIMEOUT;
      } else {
        // Error happened.
        ret = SDS_ERROR_IO;
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
  \fn          int32_t sdsioClientReceive (uint8_t *buf, uint32_t buf_size, sdsioReceiveMode_t mode)
  \brief       Receive data from SDSIO-Server in blocking or non-blocking mode.
  \param[out]  buf          pointer to the buffer where received data will be stored
  \param[in]   buf_size     buffer size in bytes
  \param[in]   mode         blocking or non-blocking mode (see \ref sdsioReceiveMode_t)
  \return      number of bytes successfully received or
               a negative value on error (see \ref SDS_Return_Codes)
*/
int32_t sdsioClientReceive (uint8_t *buf, uint32_t buf_size, sdsioReceiveMode_t mode) {
  int32_t num = 0U;
  int32_t ret = SDS_ERROR_IO;
  int32_t sock_status;

  if (mode == sdsioReceiveNonBlocking) {
    // Not supported yet
    return SDS_ERROR_IO;
  }

  while (num < buf_size) {
    sock_status = iotSocketRecv(socket, buf + num, buf_size - num);
    if (sock_status >= 0) {
      num += sock_status;
    } else {
      if (sock_status == IOT_SOCKET_EAGAIN) {
        // Timeout happened.
        ret = SDS_ERROR_TIMEOUT;
      } else {
        // Error happened.
        ret = SDS_ERROR_IO;
      }
      break;
    }
  }
  if (num != 0U) {
    ret = num;
  }
  return ret;
}
