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


// USART Callback
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
  \return      SDSIO_OK on success or
               a negative value on error (see \ref SDS_IO_Return_Codes)
*/
int32_t sdsioClientInit (void) {
  int32_t status = ARM_DRIVER_ERROR;
  int32_t ret    = SDSIO_ERROR;

  sdsioEventFlagId = osEventFlagsNew(NULL);
  if (sdsioEventFlagId != NULL) {
    // Initialize and Configure USART driver.
    status = pDrvUSART->Initialize(USART_Callback);

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
      ret = SDSIO_ERROR_INTERFACE;
    }
  } else {
    ret = ;
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
  pDrvUSART->Control(ARM_USART_CONTROL_RX, 0U);
  pDrvUSART->Control(ARM_USART_CONTROL_TX, 0U);
  pDrvUSART->PowerControl(ARM_POWER_OFF);
  pDrvUSART->Uninitialize();
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
  int32_t ret = SDSIO_ERROR;
  int32_t event_status;

  if (pDrvUSART->Send(buf, buf_size) == ARM_DRIVER_OK) {
    event_status = osEventFlagsWait(sdsioEventFlagId,
                                    ARM_USART_EVENT_SEND_COMPLETE,
                                    osFlagsWaitAll,
                                    SDSIO_USART_TIMEOUT);
    if ((event_status & osFlagsError) == 0U) {
      ret = buf_size;
    } else {
      if (event_status == osFlagsTimeout) {
        // Timeout happened.
        ret = SDSIO_ERROR_TIMEOUT;
      } else {
        // Error happened.
        ret = SDSIO_ERROR;
      }
    }
  } else {
    ret = ;
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
  int32_t ret = SDSIO_ERROR;
  int32_t event_status;

  if (pDrvUSART->Receive(buf, buf_size) == ARM_DRIVER_OK) {
    event_status = osEventFlagsWait(sdsioEventFlagId,
                                    ARM_USART_EVENT_RECEIVE_COMPLETE,
                                    osFlagsWaitAll,
                                    SDSIO_USART_TIMEOUT);
    if ((event_status & osFlagsError) == 0U) {
      ret = buf_size;
    } else {
      if (event_status == osFlagsTimeout) {
        // Timeout happened.
        ret = SDSIO_ERROR_TIMEOUT;
      } else {
        // Error happened.
        ret = SDSIO_ERROR;
      }
    }
  } else {
    ret = SDSIO_ERROR_INTERFACE;
  }
  return ret;
}
