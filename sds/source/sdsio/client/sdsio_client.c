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

// SDS I/O Client

#include <string.h>

#include "cmsis_os2.h"

#include "sdsio.h"
#include "sdsio_client.h"

static uint8_t sdsio_client_initialized = 0U;

// Ping Server retries
#ifndef SDSIO_CLIENT_PING_RETRY
#define SDSIO_CLIENT_PING_RETRY         10U
#endif

// Lock function
#ifndef SDSIO_CLIENT_NO_LOCK

#ifndef SDSIO_CLIENT_LOCK_TIMEOUT
#define SDSIO_CLIENT_LOCK_TIMEOUT       5000U
#endif

static osMutexId_t lock_id;
static inline int32_t sdsioLockCreate (void) {
  lock_id = osMutexNew(NULL);
  if (lock_id != NULL) {
    return SDSIO_OK;
  } else {
    return SDSIO_ERROR;
  }
}
static inline int32_t sdsioLockDelete (void) {
  osMutexDelete(lock_id);
  return SDSIO_OK;
}
static inline int32_t sdsioLock (void) {
  osStatus_t  status;

  status = osMutexAcquire(lock_id, SDSIO_CLIENT_LOCK_TIMEOUT);
  if (status == osOK) {
    return SDSIO_OK;
  } else if (status == osErrorTimeout) {
    return SDSIO_ERROR_TIMEOUT;
  } else {
    return SDSIO_ERROR;
  }

}
static inline int32_t sdsioUnLock (void) {
  osMutexRelease(lock_id);
  return SDSIO_OK;
}
#else
static inline void sdsioLockCreate (void) { return SDSIO_OK;}
static inline void sdsioLockDelete (void) { return SDSIO_OK;}
static inline void sdsioLock       (void) { return SDSIO_OK;}
static inline void sdsioUnLock     (void) { return SDSIO_OK;}
#endif

// Internal helper functions
/**
  Ping Server to check if it is active.
  Send:
    header: command   = SDSIO_CMD_PING
            sdsio_id  = not used
            argument  = not used
            data_size = 0
    data:   no data
  Receive:
    header: command   = SDSIO_CMD_PING
            sdsio_id  = not used
            argument  = nonzero = server is active
            data_size = 0
    data:   no data
*/
static int32_t PingServer (void) {
  int32_t  ret = SDSIO_ERROR;
  header_t header;

  header.command   = SDSIO_CMD_PING;
  header.sdsio_id  = 0U;
  header.argument  = 0U;
  header.data_size = 0U;

  // Send Header
  ret = sdsioClientSend((const uint8_t *)&header, sizeof(header_t));
  if (ret == sizeof(header_t)) {
    // Receive header.
    ret = sdsioClientReceive((uint8_t *)&header, sizeof(header_t));
    if (ret == sizeof(header_t)) {
      if ((header.command   == SDSIO_CMD_PING) &&
          (header.argument  != 0U)  &&
          (header.data_size == 0U)) {
        ret = SDSIO_OK;
      }
    } else if (ret >= 0) {
      // Incomplete header received.
      ret = SDSIO_ERROR_INTERFACE;
    }
  } else if (ret >= 0) {
    // Incomplete header sent.
    ret = SDSIO_ERROR_INTERFACE;
  }

  return ret;
}


// SDS I/O functions

/** Initialize I/O interface */
int32_t sdsioInit (void) {
  int32_t  ret;
  uint32_t n;

  if (sdsio_client_initialized != 0U) {
    // SDS I/O Client already initialized.
    return SDSIO_OK;
  }

  ret = sdsioLockCreate();

  if (ret == SDSIO_OK) {
    ret = sdsioClientInit();
  }
  if (ret == SDSIO_OK) {
    for (n = 0U; n < SDSIO_CLIENT_PING_RETRY; n++){
      ret = PingServer();
      if (ret == SDSIO_OK) {
        break;
      }
      osDelay(100U);
    }
    if (n == SDSIO_CLIENT_PING_RETRY) {
      // SDS I/O Server not active.
      ret = SDSIO_ERROR_NO_SERVER;
    }
  }
  if (ret != SDSIO_OK) {
    sdsioLockDelete();
  } else {
    sdsio_client_initialized = 1U;
  }

  return ret;
}

/** Un-initialize I/O interface */
int32_t sdsioUninit (void) {

  if (sdsio_client_initialized == 0U) {
    // SDS I/O Client not initialized.
    return SDSIO_OK;
  }
  sdsioClientUninit();
  sdsioLockDelete();
  sdsio_client_initialized = 0U;

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
  int32_t  ret = SDSIO_ERROR;
  uint32_t data_size;
  header_t header;

  if (sdsio_client_initialized == 0U) {
    // SDS I/O Client not initialized.
    return NULL;
  }

  if (name != NULL) {
    if (sdsioLock() == SDSIO_OK) {
      data_size = strlen(name) + 1U;
      header.command   = SDSIO_CMD_OPEN;
      header.sdsio_id  = 0U;
      header.argument  = mode;
      header.data_size = data_size;

      // Send header.
      ret = sdsioClientSend((const uint8_t *)&header, sizeof(header_t));
      if (ret == sizeof(header_t)) {
        // Send data.
        ret = sdsioClientSend((const uint8_t *)name, data_size);
      }
      // Receive header.
      if (ret == data_size) {
        ret = sdsioClientReceive((uint8_t *)&header, sizeof(header_t));
        if (ret == sizeof(header_t)) {
          if ((header.command   == SDSIO_CMD_OPEN) &&
              (header.argument  == mode)           &&
              (header.data_size == 0U)) {
            sdsio_id = header.sdsio_id;
          }
        }
      }

      sdsioUnLock();
    }
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

  if (sdsio_client_initialized == 0U) {
    // SDS I/O Client not initialized.
    return SDSIO_ERROR;
  }

  if (id != NULL) {
    ret = sdsioLock();
    if (ret == SDSIO_OK) {
      header.command   = SDSIO_CMD_CLOSE;
      header.sdsio_id  = (uint32_t)id;
      header.argument  = 0U;
      header.data_size = 0U;

      // Send Header.
      ret = sdsioClientSend((const uint8_t *)&header, sizeof(header_t));
      if (ret == sizeof(header_t)) {
        ret = SDSIO_OK;
      } else if (ret >= 0) {
        // Incomplete header sent.
        ret = SDSIO_ERROR_INTERFACE;
      }
      sdsioUnLock();
    }
  } else {
    // Invalid parameter.
    ret = SDSIO_ERROR_PARAMETER;
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
int32_t sdsioWrite (sdsioId_t id, const void *buf, uint32_t buf_size) {
  int32_t  ret = SDSIO_ERROR;
  header_t header;

  if (sdsio_client_initialized == 0U) {
    // SDS I/O Client not initialized.
    return SDSIO_ERROR;
  }

  if ((id != NULL) && (buf != NULL) && (buf_size != 0U)) {
    ret = sdsioLock();
    if (ret == SDSIO_OK) {
      header.command   = SDSIO_CMD_WRITE;
      header.sdsio_id  = (uint32_t)id;
      header.argument  = 0U;
      header.data_size = buf_size;

      // Send header.
      ret = sdsioClientSend((const uint8_t *)&header, sizeof(header_t));
      if (ret == sizeof(header_t)) {
        // Send data.
        ret = sdsioClientSend((const uint8_t *)buf, buf_size);
        if ((ret >= 0) && (ret < buf_size)) {
          // Incomplete data sent.
          ret = SDSIO_ERROR_INTERFACE;
        }
      } else if (ret >= 0) {
        // Incomplete header sent.
        ret = SDSIO_ERROR_INTERFACE;
      }
      sdsioUnLock();
    }
  } else {
    // Invalid parameter.
    ret = SDSIO_ERROR_PARAMETER;
  }

  return ret;
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
            argument  = nonzero = end of stream, else 0
            data_size = number of data bytes read
    data    data read
*/
int32_t sdsioRead (sdsioId_t id, void *buf, uint32_t buf_size) {
  int32_t  ret = SDSIO_ERROR;
  uint32_t size;
  header_t header;

  if (sdsio_client_initialized == 0U) {
    // SDS I/O Client not initialized.
    return SDSIO_ERROR;
  }

  if ((id != NULL) && (buf != NULL) && (buf_size != 0U)) {
    ret = sdsioLock();
    if (ret == SDSIO_OK) {
      header.command   = SDSIO_CMD_READ;
      header.sdsio_id  = (uint32_t)id;
      header.argument  = buf_size;
      header.data_size = 0U;

      // Send header.
      ret = sdsioClientSend((const uint8_t *)&header, sizeof(header_t));
      if (ret == sizeof(header_t)) {
        // Receive header
        ret = sdsioClientReceive((uint8_t *)&header, sizeof(header_t));
        // Check if full header is received.
        if (ret == sizeof(header_t)) {
          // Check if header is valid.
          if ((header.command == SDSIO_CMD_READ) && (header.sdsio_id == (uint32_t)id)) {
            if (header.data_size == 0) {
              if (header.argument != 0U) {
                // End of stream.
                ret = SDSIO_EOS;
              } else {
                // No data available.
                ret = 0;
              }
            } else {
              if (header.data_size < buf_size) {
                size = header.data_size;
              } else {
                size = buf_size;
              }
              // Read data.
              ret = sdsioClientReceive((uint8_t *)buf, size);
            }
          } else {
            // Invalid header received.
            ret = SDSIO_ERROR_INTERFACE;
          }
        } else if (ret >= 0) {
          // Incomplete header received.
          ret = SDSIO_ERROR_INTERFACE;
        }
      } else if (ret >= 0) {
        // Incomplete header sent.
        ret = SDSIO_ERROR_INTERFACE;
      }
      sdsioUnLock();
    }
  } else {
    // Invalid parameter.
    ret = SDSIO_ERROR_PARAMETER;
  }

  return ret;
}
