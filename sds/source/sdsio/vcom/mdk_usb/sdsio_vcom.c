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

// SDS I/O interface - virtual COM port

#include <string.h>

#ifndef SDSIO_NO_LOCK
#include "cmsis_os2.h"
#endif

#include "rl_usb.h"                     // Keil.MDK-Plus::USB:CORE
#include "sdsio.h"

#ifndef SDSIO_USB_DEVICE_INDEX
#define SDSIO_USB_DEVICE_INDEX  0U
#endif

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

static CDC_LINE_CODING cdc_acm_line_coding = { 0U, 0U, 0U, 0U };
// Called upon USB Host request to change communication settings.ed or not processed.
bool USBD_CDC0_ACM_SetLineCoding (const CDC_LINE_CODING *line_coding) {
  cdc_acm_line_coding = *line_coding;
  return true;
}
// Called upon USB Host request to retrieve communication settings.
bool USBD_CDC0_ACM_GetLineCoding (CDC_LINE_CODING *line_coding) {
  *line_coding = cdc_acm_line_coding;
  return true;
}

/**
  \fn          uint32_t sdsioSend (const void *buf, uint32_t buf_size)
  \brief       Send data via iot vcom
  \param[in]   buf          pointer to buffer with data to send
  \param[in]   buf_size     buffer size in bytes
  \return      number of bytes sent
*/
static uint32_t sdsioSend (const void *buf, uint32_t buf_size) {
  int32_t  status;
  uint32_t num = 0U;

  while (num < buf_size) {
    status = USBD_CDC_ACM_WriteData(SDSIO_USB_DEVICE_INDEX,
                                    (const uint8_t *)buf + num,
                                    buf_size - num);
    if (status >= 0) {
      num += (uint32_t)status;
    } else {
      // Error
      if ((status != usbDriverBusy) && (status != usbTimeout)) {
        break;
      }
    }
  }

  return num;
}

/**
  \fn          uint32_t sdsioReceive (void *buf, uint32_t buf_size)
  \brief       Receive data via iot vcom
  \param[out]  buf          pointer to buffer for data to read
  \param[in]   buf_size     buffer size in bytes
  \return      number of bytes received
*/
static uint32_t sdsioReceive (void *buf, uint32_t buf_size) {
  int32_t  status;
  uint32_t num = 0U;

  while (num < buf_size) {
    status = USBD_CDC_ACM_ReadData(SDSIO_USB_DEVICE_INDEX,
                                   (uint8_t *)buf + num,
                                   buf_size - num);
    if (status >= 0) {
      num += (uint32_t)status;
    } else {
      // Error
      if ((status != usbDriverBusy) && (status != usbTimeout)) {
        break;
      }
    }
  }

  return num;
}


// SDS I/O functions

/** Initialize I/O interface */
int32_t sdsioInit (void) {
  int32_t ret  = SDSIO_ERROR;

  sdsioLockCreate();
  if (USBD_Initialize(SDSIO_USB_DEVICE_INDEX) == usbOK) {
    if (USBD_Connect(SDSIO_USB_DEVICE_INDEX) == usbOK) {
      while (USBD_Configured(SDSIO_USB_DEVICE_INDEX) == false);
      ret = SDSIO_OK;
    }
  }
  if (ret == SDSIO_ERROR) {
    sdsioLockDelete();
  }

  return ret;
}

/** Un-initialize I/O interface */
int32_t sdsioUninit (void) {
  USBD_Disconnect(SDSIO_USB_DEVICE_INDEX);
  USBD_Uninitialize(SDSIO_USB_DEVICE_INDEX);
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
  header_t header;
  uint32_t size;
  uint32_t sdsio_id = 0U;

  if (name != NULL) {
    sdsioLock();

    header.command   = SDSIO_CMD_OPEN;
    header.sdsio_id  = 0U;
    header.argument  = mode;
    header.data_size = strlen(name) + 1U;

    // Send header
    size = sizeof(header_t);
    if (sdsioSend(&header, size) == size) {

      // Send stream name
      size = header.data_size;
      if (sdsioSend(name, size) == size) {

        // Receive header
        size = sizeof(header_t);
        if (sdsioReceive(&header, size) == size) {
          if ((header.command   == SDSIO_CMD_OPEN) &&
              (header.argument  == mode)           &&
              (header.data_size == 0U)) {
            sdsio_id = header.sdsio_id;
          }
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
  header_t header;
  uint32_t size;
  int32_t  ret = SDSIO_ERROR;

  if (id != NULL) {
    sdsioLock();

    header.command   = SDSIO_CMD_CLOSE;
    header.sdsio_id  = (uint32_t)id;
    header.argument  = 0U;
    header.data_size = 0U;

    // Send Header
    size = sizeof(header_t);
    if (sdsioSend(&header, size) == size) {
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
  header_t header;
  uint32_t size;
  uint32_t num = 0U;

  if ((id != NULL) && (buf != NULL) && (buf_size != 0U)) {
    sdsioLock();

    header.command   = SDSIO_CMD_WRITE;
    header.sdsio_id  = (uint32_t)id;
    header.argument  = 0U;
    header.data_size = buf_size;

    // Send header
    size = sizeof(header_t);
    if (sdsioSend(&header, size) == size) {

      // Send Data
      if (sdsioSend(buf, buf_size) == buf_size) {
        num = buf_size;
      }
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
  header_t header;
  uint32_t size;
  uint32_t num = 0U;

  if ((id != NULL) && (buf != NULL) && (buf_size != 0U)) {
    sdsioLock();

    header.command   = SDSIO_CMD_READ;
    header.sdsio_id  = (uint32_t)id;
    header.argument  = buf_size;
    header.data_size = 0U;

    // Send header
    size = sizeof(header_t);
    if (sdsioSend(&header, size) == size) {

      // Receive header
      if (sdsioReceive(&header, size) == size) {
        if ((header.command   == SDSIO_CMD_READ) &&
            (header.sdsio_id  == (uint32_t)id)   &&
            (header.data_size <= buf_size)) {

          // Receive data
          size = header.data_size;
          if (sdsioReceive(buf, size) == size) {
            num = size;
          }
        }
      }
    }

    sdsioUnLock();
  }

  return num;
}
