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

#include <stdbool.h>
#include <string.h>

#include "cmsis_os2.h"

#include "sds.h"
#include "sdsio.h"
#include "sdsio_client.h"

 // SDSIO header
typedef struct {
  uint32_t command;
  uint32_t sdsio_id;
  uint32_t argument;
  uint32_t data_size;
} sdsio_header_t;

// SDSIO Server Command IDs
#define SDSIO_CMD_OPEN          1U
#define SDSIO_CMD_CLOSE         2U
#define SDSIO_CMD_WRITE         3U
#define SDSIO_CMD_READ          4U
#define SDSIO_CMD_PING          5U
#define SDSIO_CMD_FLAGS         6U
#define SDSIO_CMD_INFO          7U

static uint8_t sdsio_client_initialized = 0U;

// Error data buffer size for sending info to the server
#ifndef SDSIO_CLIENT_ERROR_MAX_DATA_SIZE
#define SDSIO_CLIENT_ERROR_MAX_DATA_SIZE  128
#endif

static volatile uint8_t sdsio_client_inactive_rx_cnt = 0U;
static          uint8_t sdsio_client_error_data[SDSIO_CLIENT_ERROR_MAX_DATA_SIZE];

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
    return SDS_OK;
  } else {
    return SDS_ERROR_IO;
  }
}
static inline int32_t sdsioLockDelete (void) {
  osMutexDelete(lock_id);
  return SDS_OK;
}
static inline int32_t sdsioLock (void) {
  osStatus_t  status;

  status = osMutexAcquire(lock_id, SDSIO_CLIENT_LOCK_TIMEOUT);
  if (status == osOK) {
    return SDS_OK;
  } else if (status == osErrorTimeout) {
    return SDS_ERROR_TIMEOUT;
  } else {
    return SDS_ERROR_IO;
  }

}
static inline int32_t sdsioUnlock (void) {
  osMutexRelease(lock_id);
  return SDS_OK;
}
#else
static inline int32_t sdsioLockCreate (void) { return SDS_OK; }
static inline int32_t sdsioLockDelete (void) { return SDS_OK; }
static inline int32_t sdsioLock       (void) { return SDS_OK; }
static inline int32_t sdsioUnlock     (void) { return SDS_OK; }
#endif

// Internal helper functions

/**
  \fn          void sdsioFlagsModify (uint32_t set_mask, uint32_t clear_mask)
  \brief       Modify SDS control flags.
  \param[in]   set_mask        bits to set in sdsFlags
  \param[in]   clear_mask      bits to clear in sdsFlags
*/
void sdsioFlagsModify (uint32_t set_mask, uint32_t clear_mask) {

  sdsFlags |=  set_mask;
  sdsFlags &= ~clear_mask;
}

/**
  \fn          int32_t sdsioClientReceiveHeader (uint8_t *buf, uint32_t buf_size)
  \brief       Receive header from SDSIO-Server.
  \param[out]  buf          pointer to the buffer where received header will be stored
  \param[in]   buf_size     buffer size in bytes
  \return      number of bytes successfully received or
               a negative value on error (see \ref SDS_IO_Return_Codes)
*/
int32_t sdsioClientReceiveHeader (uint8_t *buf, uint32_t buf_size) {
  int32_t        ret;
  uint32_t       set_mask;
  uint32_t       clr_mask;
  sdsio_header_t header;

  if (buf_size < sizeof(header)) {
    return SDS_ERROR_PARAMETER;
  }

  do {
    // Receive header
    ret = sdsioClientReceive((uint8_t *)&header, sizeof(header), sdsioReceiveBlocking);
    if (ret == sizeof(header)) {
      // If header is flags response, process it and update (modify) flags
      if (header.command == SDSIO_CMD_FLAGS) {
        if (header.data_size == 0U) {
          set_mask = header.sdsio_id;
          clr_mask = header.argument;
          sdsioFlagsModify (set_mask, clr_mask);
          sdsio_client_inactive_rx_cnt = 0U;
        }
      } else {
        // Not async flags response but expected response to a command, return it
        memcpy(buf, &header, sizeof(header));
        break;
      }
    } else if (ret >= 0) {
      // Incomplete header received.
      ret = SDS_ERROR_IO;
      break;
    }
  } while (true);

  return ret;
}

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
  int32_t        ret = SDS_ERROR_IO;
  sdsio_header_t header;

  header.command   = SDSIO_CMD_PING;
  header.sdsio_id  = 0U;
  header.argument  = 0U;
  header.data_size = 0U;

  // Send Header
  ret = sdsioClientSend((const uint8_t *)&header, sizeof(header));
  if (ret == sizeof(header)) {
    // Receive header.
    ret = sdsioClientReceiveHeader((uint8_t *)&header, sizeof(header));
    if (ret == sizeof(header)) {
      if ((header.command   == SDSIO_CMD_PING) &&
          (header.argument  != 0U)  &&
          (header.data_size == 0U)) {
        ret = SDS_OK;
      }
    } else if (ret >= 0) {
      // Incomplete header received.
      ret = SDS_ERROR_IO;
    }
  } else if (ret >= 0) {
    // Incomplete header sent.
    ret = SDS_ERROR_IO;
  }

  return ret;
}


// SDS I/O functions

/**
  Initialize SDS I/O Interface.
*/
int32_t sdsioInit (void) {
  int32_t  ret;
  uint32_t n;

  if (sdsio_client_initialized != 0U) {
    // SDS I/O Client already initialized.
    return SDS_OK;
  }

  ret = sdsioLockCreate();

  if (ret == SDS_OK) {
    ret = sdsioClientInit();
  }
  if (ret != SDS_OK) {
    sdsioLockDelete();
  } else {
    sdsio_client_initialized = 1U;
  }

  return ret;
}

/**
  Un-initialize SDS I/O Interface.
*/
int32_t sdsioUninit (void) {

  if (sdsio_client_initialized == 0U) {
    // SDS I/O Client not initialized.
    return SDS_OK;
  }
  sdsioClientUninit();
  sdsioLockDelete();
  sdsio_client_initialized = 0U;

  return SDS_OK;
}

/**
  Open I/O stream.
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
  uint32_t       sdsio_id = 0U;
  int32_t        ret = SDS_ERROR_IO;
  uint32_t       data_size;
  sdsio_header_t header;

  if (sdsio_client_initialized == 0U) {
    // SDS I/O Client not initialized.
    return NULL;
  }

  if (name != NULL) {
    if (sdsioLock() == SDS_OK) {
      data_size = strlen(name) + 1U;
      header.command   = SDSIO_CMD_OPEN;
      header.sdsio_id  = 0U;
      header.argument  = mode;
      header.data_size = data_size;

      // Send header.
      ret = sdsioClientSend((const uint8_t *)&header, sizeof(header));
      if (ret == sizeof(header)) {
        // Send data.
        ret = sdsioClientSend((const uint8_t *)name, data_size);
      }
      // Receive header.
      if (ret == data_size) {
        ret = sdsioClientReceiveHeader((uint8_t *)&header, sizeof(header));
        if (ret == sizeof(header)) {
          if ((header.command   == SDSIO_CMD_OPEN) &&
              (header.argument  == mode)           &&
              (header.data_size == 0U)) {
            sdsio_id = header.sdsio_id;
          }
        }
      }

      sdsioUnlock();
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
  int32_t        ret = SDS_ERROR_IO;
  sdsio_header_t header;

  if (sdsio_client_initialized == 0U) {
    // SDS I/O Client not initialized.
    return SDS_ERROR_IO;
  }

  if (id != NULL) {
    ret = sdsioLock();
    if (ret == SDS_OK) {
      header.command   = SDSIO_CMD_CLOSE;
      header.sdsio_id  = (uint32_t)id;
      header.argument  = 0U;
      header.data_size = 0U;

      // Send Header.
      ret = sdsioClientSend((const uint8_t *)&header, sizeof(header));
      if (ret == sizeof(header)) {
        ret = SDS_OK;
      } else if (ret >= 0) {
        // Incomplete header sent.
        ret = SDS_ERROR_IO;
      }
      sdsioUnlock();
    }
  } else {
    // Invalid parameter.
    ret = SDS_ERROR_PARAMETER;
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
  int32_t        ret = SDS_ERROR_IO;
  sdsio_header_t header;

  if (sdsio_client_initialized == 0U) {
    // SDS I/O Client not initialized.
    return SDS_ERROR;
  }

  if ((id != NULL) && (buf != NULL) && (buf_size != 0U)) {
    ret = sdsioLock();
    if (ret == SDS_OK) {
      header.command   = SDSIO_CMD_WRITE;
      header.sdsio_id  = (uint32_t)id;
      header.argument  = 0U;
      header.data_size = buf_size;

      // Send header.
      ret = sdsioClientSend((const uint8_t *)&header, sizeof(header));
      if (ret == sizeof(header)) {
        // Send data.
        ret = sdsioClientSend((const uint8_t *)buf, buf_size);
        if ((ret >= 0) && (ret < buf_size)) {
          // Incomplete data sent.
          ret = SDS_ERROR_IO;
        }
      } else if (ret >= 0) {
        // Incomplete header sent.
        ret = SDS_ERROR_IO;
      }
      sdsioUnlock();
    }
  } else {
    // Invalid parameter.
    ret = SDS_ERROR_PARAMETER;
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
  int32_t        ret = SDS_ERROR_IO;
  uint32_t       size;
  sdsio_header_t header;

  if (sdsio_client_initialized == 0U) {
    // SDS I/O Client not initialized.
    return SDS_ERROR_IO;
  }

  if ((id != NULL) && (buf != NULL) && (buf_size != 0U)) {
    ret = sdsioLock();
    if (ret == SDS_OK) {
      header.command   = SDSIO_CMD_READ;
      header.sdsio_id  = (uint32_t)id;
      header.argument  = buf_size;
      header.data_size = 0U;

      // Send header.
      ret = sdsioClientSend((const uint8_t *)&header, sizeof(header));
      if (ret == sizeof(header)) {
        // Receive header
        ret = sdsioClientReceiveHeader((uint8_t *)&header, sizeof(header));
        // Check if full header is received.
        if (ret == sizeof(header)) {
          // Check if header is valid.
          if ((header.command == SDSIO_CMD_READ) && (header.sdsio_id == (uint32_t)id)) {
            if (header.data_size == 0) {
              if (header.argument != 0U) {
                // End of stream.
                ret = SDS_EOS;
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
              ret = sdsioClientReceive((uint8_t *)buf, size, sdsioReceiveBlocking);
            }
          } else {
            // Invalid header received.
            ret = SDS_ERROR_IO;
          }
        } else if (ret >= 0) {
          // Incomplete header received.
          ret = SDS_ERROR_IO;
        }
      } else if (ret >= 0) {
        // Incomplete header sent.
        ret = SDS_ERROR_IO;
      }
      sdsioUnlock();
    }
  } else {
    // Invalid parameter.
    ret = SDS_ERROR_PARAMETER;
  }

  return ret;
}

/**
  Check whether asynchronous SDSIO_CMD_FLAGS information has been received
  from the host, and update sdsFlags accordingly.
  Read:
    header: command   = SDSIO_CMD_FLAGS
            sdsio_id  = set mask
            argument  = clear mask
            data_size = 0

  Send the current sdsFlags value, along with sdsIdleRate and any optional
  error information (sdsError), to the host.
  Send:
    header: command   = SDSIO_CMD_INFO
            sdsio_id  = sdsFlags
            argument  = sdsIdleRate
            data_size = number of error data bytes to send
    data:   error data to be sent
*/
int32_t sdsExchange (void) {
  int32_t        ret;
  uint32_t       set_mask;
  uint32_t       clr_mask;
  uint32_t       ofs = 0U;
  uint32_t       len = 0U;
  sdsio_header_t header;

  if (sdsio_client_initialized == 0U) {
    // SDS I/O Client not initialized.
    return SDS_ERROR_IO;
  }

  ret = sdsioLock();
  if (ret != SDS_OK) {
    return ret;
  }

  // Check inactivity.
  // If asynchronous response with ID = 6 (SDSIO_CMD_FLAGS) was not received in 10 calls of this function
  // then clear ALIVE flag in sdsFlags.
  if (sdsio_client_inactive_rx_cnt < 10U) {
    sdsio_client_inactive_rx_cnt++;
    if (sdsio_client_inactive_rx_cnt == 10U) {
      sdsioFlagsModify(0U, SDS_FLAG_ALIVE);
    }
  }

  do {
    // Check if asynchronous response with ID = 6 (SDSIO_CMD_FLAGS) was received
    // and if it was then repeat to drain asynchronous responses if there are multiple
    ret = sdsioClientReceive((uint8_t *)&header, sizeof(header), sdsioReceiveNonBlocking);
    if (ret == sizeof(header)) {
      // Process the flags response
      if ((header.command == SDSIO_CMD_FLAGS) && (header.data_size == 0U)) {
        set_mask = header.sdsio_id;
        clr_mask = header.argument;
        sdsioFlagsModify(set_mask, clr_mask);
        sdsio_client_inactive_rx_cnt = 0U;
      } else {
        // Invalid header received.
        ret = SDS_ERROR_IO;
      }
    } else if (ret >= 0) {
      // Incomplete header received.
      ret = SDS_ERROR_IO;
    }
  } while (ret == sizeof(header));

  if ((sdsFlags & SDS_FLAG_ALIVE) != 0U) {      // Send info only if Server is alive
    // Prepare and send Command with ID = 7 (SDSIO_CMD_INFO)
    header.command   = SDSIO_CMD_INFO;
    header.sdsio_id  = sdsFlags;
    header.argument  = sdsIdleRate;
    header.data_size = 0U;
  
    if (sdsError.occurred != 0U) {
      sdsError.occurred = 0U;
      memcpy(sdsio_client_error_data,       &sdsError.status, sizeof(sdsError.status)); ofs  = sizeof(sdsError.status);
      memcpy(sdsio_client_error_data + ofs, &sdsError.line,   sizeof(sdsError.line));   ofs += sizeof(sdsError.line);
      len = strlen(sdsError.file);
      if (len > (sizeof(sdsio_client_error_data) - ofs)) {
        len = sizeof(sdsio_client_error_data) - ofs;
      }
      memcpy(sdsio_client_error_data + ofs, sdsError.file,    len);                     ofs += len;
      header.data_size = ofs;
    }
  
    // Send header.
    ret = sdsioClientSend((const uint8_t *)&header, sizeof(header));
    if (ret == sizeof(header)) {
      if (header.data_size != 0U) {
        // Send data.
        ret = sdsioClientSend((const uint8_t *)sdsio_client_error_data, header.data_size);
        if ((ret >= 0) && (ret < header.data_size)) {
          // Incomplete data sent.
          ret = SDS_ERROR_IO;
        }
      }
    } else if (ret >= 0) {
      // Incomplete header sent.
      ret = SDS_ERROR_IO;
    }
  }

  sdsioUnlock();

  return ret;
}

/**
  \fn          void sdsFlagsModify (uint32_t set_mask, uint32_t clear_mask)
  \brief       Modify SDS control flags (atomic operation).
  \param[in]   set_mask        bits to set in sdsFlags
  \param[in]   clear_mask      bits to clear in sdsFlags
*/
void sdsFlagsModify (uint32_t set_mask, uint32_t clear_mask) {

  if (sdsioLock() == SDS_OK) {
    sdsioFlagsModify(set_mask, clear_mask);
    sdsioUnlock();
  }
}
