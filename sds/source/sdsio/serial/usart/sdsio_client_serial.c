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

// SDS I/O Client via Serial Port (CMSIS Driver:USART)

#include "cmsis_os2.h"

#include "sdsio.h"
#include "sdsio_client.h"

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
  \fn          int32_t sdsioClientInit (void)
  \brief       Initialize SDS I/O Client via CMSIS Driver:USART
  \return      SDIOS_OK: initialization success
               SDSIO_ERROR: initialization failed
*/
int32_t sdsioClientInit (void) {
  int32_t status = ARM_DRIVER_ERROR;
  int32_t ret = SDSIO_ERROR;

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
  pDrvUSART->Control(ARM_USART_CONTROL_RX, 0U);
  pDrvUSART->Control(ARM_USART_CONTROL_TX, 0U);
  pDrvUSART->PowerControl(ARM_POWER_OFF);
  pDrvUSART->Uninitialize();
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
  \fn          uint32_t sdsioClientReceive (header_t *header, void *data, uint32_t data_size)
  \brief       Receive data from SDSIO-Server
  \param[out]  header       pointer to header
  \param[out]  data         pointer to buffer for data to read
  \param[in]   data_size    data size in bytes
  \return      number of bytes received (including header)
*/
uint32_t sdsioClientReceive (header_t *header, void *data, uint32_t data_size) {
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
