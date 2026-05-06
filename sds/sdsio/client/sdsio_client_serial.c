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

// SDS I/O Client via Serial (CMSIS Driver:USART)

#include <string.h>
#include "cmsis_os2.h"
#include "cmsis_compiler.h"

#include "sds.h"
#include "sdsio_client.h"

#include "Driver_USART.h"
#include "sdsio_client_serial_config.h"

// Check configuration
#if   ((SDSIO_USART_RX_BUF_SIZE & (SDSIO_USART_RX_BUF_SIZE - 1)) != 0)
#error "SDSIO_USART_RX_BUF_SIZE must be a power of 2."
#endif

// Expansion macro used to create CMSIS Driver references
#define EXPAND_SYMBOL(name, port)   name##port
#define CREATE_SYMBOL(name, port)   EXPAND_SYMBOL(name, port)

// CMSIS-Driver USART reference (Driver_USART#)
#define CMSIS_USART_DRIVER          CREATE_SYMBOL(Driver_USART, SDSIO_USART_DRIVER_NUMBER)

// Extern CMSIS-Driver USART
extern ARM_DRIVER_USART             CMSIS_USART_DRIVER;

static ARM_DRIVER_USART *pDrvUSART = &CMSIS_USART_DRIVER;

static osEventFlagsId_t sdsioSendEventFlagId;

// USART internal receive buffer and variables
static          uint8_t  rx_buf[SDSIO_USART_RX_BUF_SIZE] __ALIGNED(32);
static volatile uint32_t rx_cnt_in;
static          uint32_t rx_cnt_out;

// USART Callback
static void USART_Callback (uint32_t event) {
  if ((event & ARM_USART_EVENT_SEND_COMPLETE) != 0U) {
    osEventFlagsSet(sdsioSendEventFlagId, ARM_USART_EVENT_SEND_COMPLETE);
  }
  if ((event & ARM_USART_EVENT_RECEIVE_COMPLETE) != 0U) {
    rx_cnt_in += sizeof(rx_buf);
    // Start receiving data into internal receive buffer
    pDrvUSART->Receive(rx_buf, sizeof(rx_buf));
  }
}


/**
  \fn          int32_t sdsioClientInit (void)
  \brief       Initialize SDSIO Client.
  \return      SDS_OK on success or
               a negative value on error (see \ref SDS_Return_Codes)
*/
int32_t sdsioClientInit (void) {
  int32_t status = ARM_DRIVER_ERROR;
  int32_t ret    = SDS_ERROR_IO;

  sdsioSendEventFlagId = osEventFlagsNew(NULL);
  if (sdsioSendEventFlagId != NULL) {
    // Initialize and Configure USART driver
    status = pDrvUSART->Initialize(USART_Callback);

    if (status == ARM_DRIVER_OK) {
      status = pDrvUSART->PowerControl(ARM_POWER_FULL);
    }
    if (status == ARM_DRIVER_OK) {
      status = pDrvUSART->Control(ARM_USART_MODE_ASYNCHRONOUS |
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
      rx_cnt_in  = 0U;
      rx_cnt_out = 0U;
      // Start reception to internal receive buffer
      status = pDrvUSART->Receive(rx_buf, sizeof(rx_buf));
    }

    if (status == ARM_DRIVER_OK) {
      ret = SDS_OK;
    } else {
      ret = SDS_ERROR_IO;
    }
  } else {
    ret = SDS_ERROR_IO;
  }

  if (ret == SDS_OK) {
    SDS_PRINTF("SDS I/O USART interface initialized successfully\n");
  } else {
    SDS_PRINTF("SDS I/O USART interface initialization failed!\n");
    SDS_PRINTF("Ensure that device is connected via USART to the host PC running SDSIO-Server, then restart the application!\n");
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
  pDrvUSART->Control(ARM_USART_CONTROL_RX, 0U);
  pDrvUSART->Control(ARM_USART_CONTROL_TX, 0U);
  pDrvUSART->PowerControl(ARM_POWER_OFF);
  pDrvUSART->Uninitialize();
  if (sdsioSendEventFlagId != NULL) {
    if (osEventFlagsDelete(sdsioSendEventFlagId) == osOK) {
      sdsioSendEventFlagId = NULL;
    }
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
  int32_t  ret = SDS_ERROR_IO;
  uint32_t event_status;

  if ((buf == NULL) || (buf_size == 0U)) {
    return SDS_ERROR_PARAMETER;
  }

  if (pDrvUSART->Send(buf, buf_size) == ARM_DRIVER_OK) {
    event_status = osEventFlagsWait(sdsioSendEventFlagId,
                                    ARM_USART_EVENT_SEND_COMPLETE,
                                    osFlagsWaitAll,
                                    SDSIO_USART_TIMEOUT);
    if ((event_status & osFlagsError) == 0U) {
      ret = buf_size;
    } else {
      if (event_status == osFlagsErrorTimeout) {
        // Timeout happened
        ret = SDS_ERROR_TIMEOUT;
      } else {
        // Error happened
        ret = SDS_ERROR_IO;
      }
    }
  } else {
    ret = SDS_ERROR_IO;
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
  uint32_t rx_cnt_in_curr;
  uint32_t rx_buf_pos;
  uint32_t rx_cnt_avail;
  uint32_t num = 0U;
  uint32_t cnt;
  uint32_t cnt_wrap;
  uint32_t tick;
  int32_t  ret = 0;

  if ((buf == NULL) || (buf_size == 0U)) {
    return SDS_ERROR_PARAMETER;
  }

  tick = osKernelGetTickCount();
  while (num < buf_size) {
    // Calculate currently available unread length of received data
    do {
      rx_cnt_in_curr = rx_cnt_in + pDrvUSART->GetRxCount();
    } while (rx_cnt_in_curr != (rx_cnt_in + pDrvUSART->GetRxCount()));
    if (rx_cnt_in_curr != rx_cnt_out) {
      rx_cnt_avail = rx_cnt_in_curr - rx_cnt_out;
    } else {
      rx_cnt_avail = 0U;
    }

    if ((mode == sdsioReceiveNonBlocking) && (rx_cnt_avail < buf_size)) {
      // For non-blocking mode: do not return partial data
      // Return exactly the requested number of bytes, and only if they are available
      break;
    }

    // Process received data up to requested size
    if (rx_cnt_avail != 0U) {
      // Calculate memory location of unread received data in rx_buf
      rx_buf_pos = rx_cnt_out & (sizeof(rx_buf)-1);

      cnt = rx_cnt_avail;
      if (rx_cnt_avail > (buf_size - num)) {
        cnt = (buf_size - num);
      }

      if ((rx_buf_pos + cnt) > sizeof(rx_buf)) {
        // If data wraps around the end of internal receive buffer
        cnt_wrap = (rx_buf_pos + cnt) - sizeof(rx_buf);
        cnt     -= cnt_wrap;
      } else {
        cnt_wrap = 0U;
      }
      memcpy(buf + num, &rx_buf[rx_buf_pos], cnt);
      if (cnt_wrap != 0U) {
        // Copy data after wrap
        memcpy(buf + num + cnt, &rx_buf[0], cnt_wrap);
        rx_cnt_out += cnt_wrap;
      }
      rx_cnt_out += cnt;
      num += cnt + cnt_wrap;
    }

    if (mode == sdsioReceiveNonBlocking) {
      // If it is non-blocking mode then exit the loop
      break;
    }

    if (rx_cnt_avail == 0U) {
      // Allow other threads to execute while no received data is available
      osDelay(1U);
    }

    if ((osKernelGetTickCount() - tick) >= SDSIO_USART_TIMEOUT) {
      // Timeout happened
      ret = SDS_ERROR_TIMEOUT;
      break;
    }
  }

  if ((ret != SDS_ERROR_TIMEOUT) && (num != 0U)) {
    ret = (int32_t)num;
  }

  return ret;
}
