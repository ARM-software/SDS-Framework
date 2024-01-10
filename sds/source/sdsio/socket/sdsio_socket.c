/*
 * Copyright (c) 2022-2024 Arm Limited. All rights reserved.
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

// SDS I/O interface via Socket (IoT Utility:Socket)

#include <string.h>
#include <stdio.h>

#ifndef SDSIO_NO_LOCK
#include "cmsis_os2.h"
#endif
#include "iot_socket.h"
#include "sdsio.h"
#include "sdsio_config_socket.h"

// SDS I/O header
typedef struct {
  uint32_t command;
  uint32_t sdsio_id;
  uint32_t argument;
  uint32_t data_size;
} header_t;

// Commands
#define SDSIO_CMD_OPEN          1U
#define SDSIO_CMD_CLOSE         2U
#define SDSIO_CMD_WRITE         3U
#define SDSIO_CMD_READ          4U
#define SDSIO_CMD_EOS           5U

static int32_t socket = -1;

// Lock function
#ifndef SDSIO_NO_LOCK
static osMutexId_t lock_id;
static inline void sdsioLockCreate (void) {
  lock_id = osMutexNew(NULL);
}
static inline void sdsioLockDelete (void) {
  osMutexDelete(lock_id);
}
static inline void sdsioLock (void) {
  osMutexAcquire(lock_id, osWaitForever);
}
static inline void sdsioUnLock (void) {
  osMutexRelease(lock_id);
}
#else
static inline void sdsioLockCreate (void) {}
static inline void sdsioLockDelete (void) {}
static inline void sdsioLock       (void) {}
static inline void sdsioUnLock     (void) {}
#endif

/**
  \fn          uint32_t sdsioSend (const header_t *header, const void *data, uint32_t data_size)
  \brief       Send data via iot socket
  \param[in]   header       pointer to header
  \param[in]   data         pointer to buffer with data to send
  \param[in]   data_size    data size in bytes
  \return      number of bytes sent (including header)
*/
static uint32_t sdsioSend (const header_t *header, const void *data, uint32_t data_size) {
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
  \fn          uint32_t sdsioReceive (header_t *header, void *data, uint32_t data_size)
  \brief       Receive data via iot socket
  \param[out]  header       pointer to header
  \param[out]  data         pointer to buffer for data to read
  \param[in]   data_size    data size in bytes
  \return      number of bytes received (including header)
*/
static uint32_t sdsioReceive (header_t *header, void *data, uint32_t data_size) {
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


// SDS I/O functions

/** Initialize I/O interface */
int32_t sdsioInit (void) {
  int32_t  ret  = SDSIO_ERROR;
  uint32_t tout = SDSIO_SOCKET_RECEIVE_TOUT;
  uint8_t  ip[4];

  if (socket == -1) {
    sdsioLockCreate();
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
    if (ret == SDSIO_ERROR) {
      sdsioLockDelete();
    }
  } else {
    // sdsio is already initialized
    ret = SDSIO_OK;
  }

  return ret;
}

/** Un-initialize I/O interface */
int32_t sdsioUninit (void) {
  iotSocketClose(socket);
  socket = -1;
  sdsioLockDelete();
  return SDSIO_OK;
}

/**
  Open I/O stream
  Send:
    header: command   = SDSIO_CMD_OPEN
            sdsio_id  = not used
            argument  = sdsioMode_t
            data_size = size of stream name
    data:   stream name
  Receive:
    header: command   = SDSIO_CMD_OPEN
            sdsio_id  = retrieved sdsio identifier
            argument  = sdsioMode_t
            data_size = 0
    data:   no data
*/
sdsioId_t sdsioOpen (const char *name, sdsioMode_t mode) {
  uint32_t sdsio_id = 0U;
  header_t header;
  uint32_t size, data_size;

  if (name != NULL) {
    sdsioLock();

    data_size = strlen(name) + 1U;
    header.command   = SDSIO_CMD_OPEN;
    header.sdsio_id  = 0U;
    header.argument  = mode;
    header.data_size = data_size;

    // Send header + data
    size = sizeof(header_t) + data_size;
    if (sdsioSend(&header, name, data_size) == size) {

      // Receive header
      size = sizeof(header_t);
      if (sdsioReceive(&header, NULL, 0U) == size) {
        if ((header.command   == SDSIO_CMD_OPEN) &&
            (header.argument  == mode)           &&
            (header.data_size == 0U)) {
          sdsio_id = header.sdsio_id;
        }
      }
    }

    sdsioUnLock();
  }

  return (sdsioId_t)sdsio_id;
}

/**
  Close I/O stream.
  Send:
    header: command   = SDSIO_CMD_CLOSE
            sdsio_id  = sdsio identifier
            argument  = not used
            data_size = 0
    data:   no data
*/
int32_t sdsioClose (sdsioId_t id) {
  int32_t  ret = SDSIO_ERROR;
  header_t header;
  uint32_t size;

  if (id != NULL) {
    sdsioLock();

    header.command   = SDSIO_CMD_CLOSE;
    header.sdsio_id  = (uint32_t)id;
    header.argument  = 0U;
    header.data_size = 0U;

    // Send Header
    size = sizeof(header_t);
    if (sdsioSend(&header, NULL, 0U) == size) {
      ret = SDSIO_OK;
    }

    sdsioUnLock();
  }

  return ret;
}

/**
  Write data to I/O stream.
  Send:
    header: command   = SDSIO_CMD_WRITE
            sdsio_id  = sdsio identifier
            argument  = not used
            data_size = number of data bytes
    data:   data to be written
*/
uint32_t sdsioWrite (sdsioId_t id, const void *buf, uint32_t buf_size) {
  uint32_t num = 0U;
  header_t header;
  uint32_t size;

  if ((id != NULL) && (buf != NULL) && (buf_size != 0U)) {
    sdsioLock();

    header.command   = SDSIO_CMD_WRITE;
    header.sdsio_id  = (uint32_t)id;
    header.argument  = 0U;
    header.data_size = buf_size;

    // Send header + data
    size = sizeof(header_t) + buf_size;
    if (sdsioSend(&header, buf, buf_size) == size) {
      num = buf_size;
    }

    sdsioUnLock();
  }

  return num;
}

/**
  Read data from I/O stream.
  Send:
    header: command   = SDSIO_CMD_READ
            sdsio_id  = sdsio identifier
            argument  = number of bytes to be read
            data_size = 0
    data:   no data
  Receive:
    header: command   = SDSIO_CMD_READ
            sdsio_id  = sdsio identifier
            argument  = not used
            data_size = number of data bytes read
    data    data read
*/
uint32_t sdsioRead (sdsioId_t id, void *buf, uint32_t buf_size) {
  uint32_t num = 0U;
  header_t header;

  if ((id != NULL) && (buf != NULL) && (buf_size != 0U)) {
    sdsioLock();

    header.command   = SDSIO_CMD_READ;
    header.sdsio_id  = (uint32_t)id;
    header.argument  = buf_size;
    header.data_size = 0U;

    // Send header
    if (sdsioSend(&header, NULL, 0U) == sizeof(header_t)) {

      // Receive header + data
      if (sdsioReceive(&header, buf, buf_size) >= sizeof(header_t)) {
        if ((header.command   == SDSIO_CMD_READ) &&
            (header.sdsio_id  == (uint32_t)id)   &&
            (header.data_size <= buf_size)) {
          num = header.data_size;
        }
      }
    }

    sdsioUnLock();
  }

  return num;
}

/**
  Check if end of stream has been reached.
  Send:
    header: command   = SDSIO_CMD_EOS
            sdsio_id  = sdsio identifier
            argument  = not used
            data_size = 0
    data:   no data
  Receive:
    header: command   = SDSIO_CMD_EOS
            sdsio_id  = sdsio identifier
            argument  = nonzero = end of stream, else 0
            data_size = 0
    data:   no data
*/
int32_t sdsioEndOfStream (sdsioId_t id) {
  int32_t  eos = 0;
  header_t header;

  if (id != NULL) {
    sdsioLock();

    header.command   = SDSIO_CMD_EOS;
    header.sdsio_id  = (uint32_t)id;
    header.argument  = 0U;
    header.data_size = 0U;

    // Send Header
    if (sdsioSend(&header, NULL, 0U) == sizeof(header_t)) {
      // Receive header
      if (sdsioReceive(&header, NULL, 0U) == sizeof(header_t)) {
        if ((header.command   == SDSIO_CMD_EOS) &&
            (header.sdsio_id  == (uint32_t)id)  &&
            (header.data_size == 0U)) {
          eos = (int32_t)header.argument;
        }
      }
    }
    sdsioUnLock();
  }

  return eos;
}
