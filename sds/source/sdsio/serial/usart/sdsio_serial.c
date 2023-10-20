/*
 * Copyright (c) 2023 Arm Limited. All rights reserved.
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

// SDS I/O interface via Serial Port (CMSIS Driver:USART)

#include <string.h>

#include "cmsis_os2.h"

#include "sdsio.h"
#include "Driver_USART.h"
#include "sdsio_config_serial_usart.h"

// Expansion macro used to create CMSIS Driver references
#define EXPAND_SYMBOL(name, port)   name##port
#define CREATE_SYMBOL(name, port)   EXPAND_SYMBOL(name, port)

// CMSIS-Driver USART reference (Driver_USART#)
#define CMSIS_USART_DRIVER          CREATE_SYMBOL(Driver_USART, SDSIO_USART_DRIVER_NUMBER)

// Extern CMSIS-Driver USART
extern ARM_DRIVER_USART             CMSIS_USART_DRIVER;

static ARM_DRIVER_USART *pDrvUSART = &CMSIS_USART_DRIVER;

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

static osEventFlagsId_t sdsioEventFlagId;

/**
  USART Callback
*/
static void USART_Callback (uint32_t event) {
  if ((event & ARM_USART_EVENT_SEND_COMPLETE) != 0U) {
    osEventFlagsSet(sdsioEventFlagId, ARM_USART_EVENT_SEND_COMPLETE);
  }
  if ((event & ARM_USART_EVENT_RECEIVE_COMPLETE) != 0U) {
    osEventFlagsSet(sdsioEventFlagId, ARM_USART_EVENT_RECEIVE_COMPLETE);
  }
}

/**
  \fn          uint32_t sdsioSend (const header_t *header, const void *data, uint32_t data_size)
  \brief       Send data via USART
  \param[in]   header       pointer to header
  \param[in]   data         pointer to buffer with data to send
  \param[in]   data_size    data size in bytes
  \return      number of bytes sent (including header)
*/
static uint32_t sdsioSend (const header_t *header, const void *data, uint32_t data_size) {
  uint32_t num, status;

  if (header == NULL) {
    return 0U;
  }

  // Send header
  num = 0U;
  if (pDrvUSART->Send((const uint8_t *)header, sizeof(header_t)) == ARM_DRIVER_OK) {
    status = osEventFlagsWait(sdsioEventFlagId,
                              ARM_USART_EVENT_SEND_COMPLETE,
                              osFlagsWaitAll,
                              SDSIO_USART_TIMEOUT);
    if ((status & osFlagsError) == 0U) {
      num = sizeof(header_t);
    }
  }

  // Send data
  if ((num != 0U) && (data != NULL) && (data_size != 0U)) {
    if (pDrvUSART->Send(data, data_size) == ARM_DRIVER_OK) {
      status = osEventFlagsWait(sdsioEventFlagId,
                                ARM_USART_EVENT_SEND_COMPLETE,
                                osFlagsWaitAll,
                                SDSIO_USART_TIMEOUT);
      if ((status & osFlagsError) == 0U) {
        num += data_size;
      }
    }
  }

  return num;
}

/**
  \fn          uint32_t sdsioReceive (header_t *header, void *data, uint32_t data_size)
  \brief       Receive data via USART
  \param[out]  header       pointer to header
  \param[out]  data         pointer to buffer for data to read
  \param[in]   data_size    data size in bytes
  \return      number of bytes received (including header)
*/
static uint32_t sdsioReceive (header_t *header, void *data, uint32_t data_size) {
  uint32_t num, size, status;

  if (header == NULL) {
    return 0U;
  }

  // Receive header
  num = 0U;
  if (pDrvUSART->Receive(header, sizeof(header_t)) == ARM_DRIVER_OK) {
    status = osEventFlagsWait(sdsioEventFlagId,
                              ARM_USART_EVENT_RECEIVE_COMPLETE,
                              osFlagsWaitAll,
                              SDSIO_USART_TIMEOUT);
    if ((status & osFlagsError) == 0U) {
      num = sizeof(header_t);
    }
  }

  // Receive data
  if ((num != 0U) && (header->data_size != 0U) &&
      (data != NULL) && (data_size != 0U)) {

    if (header->data_size < data_size) {
      size = header->data_size;
    } else {
      size = data_size;
    }
    if (pDrvUSART->Receive(data, size) == ARM_DRIVER_OK) {
      status = osEventFlagsWait(sdsioEventFlagId,
                                ARM_USART_EVENT_RECEIVE_COMPLETE,
                                osFlagsWaitAll,
                                SDSIO_USART_TIMEOUT);
      if ((status & osFlagsError) == 0U) {
        num += data_size;
      }
    }
  }

  return num;
}


// SDS I/O functions

/** Initialize I/O interface */
int32_t sdsioInit (void) {
  int32_t status = ARM_DRIVER_ERROR;
  int32_t ret;

  sdsioLockCreate();
  sdsioEventFlagId = osEventFlagsNew(NULL);

  if (sdsioEventFlagId != NULL) {
    status = pDrvUSART->Initialize(USART_Callback);
  }
  if (status == ARM_DRIVER_OK) {
    status = pDrvUSART->PowerControl(ARM_POWER_FULL);
  }
  if (status == ARM_DRIVER_OK) {
    pDrvUSART->Control(ARM_USART_MODE_ASYNCHRONOUS |
                       SDSIO_USART_DATA_BITS       |
                       SDSIO_USART_PARITY          |
                       SDSIO_USART_STOP_BITS,
                       SDSIO_USART_BAUDRATE);
  }
  if (status == ARM_DRIVER_OK) {
    status = pDrvUSART->Control(ARM_USART_CONTROL_RX, 1U);
  }
  if (status == ARM_DRIVER_OK) {
    status = pDrvUSART->Control(ARM_USART_CONTROL_TX, 1U);
  }
  if (status == ARM_DRIVER_OK) {
    ret = SDSIO_OK;
  } else {
    sdsioLockDelete();
    ret = SDSIO_ERROR;
  }
  return ret;
}

/** Un-initialize I/O interface */
int32_t sdsioUninit (void) {
  pDrvUSART->Control(ARM_USART_CONTROL_RX, 0U);
  pDrvUSART->Control(ARM_USART_CONTROL_TX, 0U);
  pDrvUSART->PowerControl(ARM_POWER_OFF);
  pDrvUSART->Uninitialize();
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
