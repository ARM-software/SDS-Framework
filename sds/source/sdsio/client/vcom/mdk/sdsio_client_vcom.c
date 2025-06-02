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

// SDS I/O Client via USB Virtual COM Port (Keil::USB:Device:CDC)

#include "rl_usb.h"                     // Keil.MDK-Plus::USB:CORE
#include "cmsis_os2.h"

#include "sdsio.h"
#include "sdsio_client.h"
#include "sdsio_config_vcom_mdk.h"

// Initialize CDC line coding structure
static CDC_LINE_CODING cdc_acm_line_coding = { 0U, 0U, 0U, 0U };

 // SDS IO event flag identifier
static osEventFlagsId_t sdsioEventFlagId;

// SDS IO event flag values
#define SDSIO_CLIENT_EVENT_DATA_SENT        (1UL << 0)
#define SDSIO_CLIENT_EVENT_DATA_RECEIVED    (1UL << 1)

// Expansion macro used to create USBD_CDCn_ACM_ Callback functions
#define EXPAND_SYMBOL(prefix, value, suffix) prefix##value##suffix
#define CREATE_SYMBOL(prefix, value, suffix) EXPAND_SYMBOL(prefix, value, suffix)

// Called upon USB Host request to change communication settings.
// bool USBD_CDCn_ACM_SetLineCoding (const CDC_LINE_CODING *line_coding)
bool CREATE_SYMBOL(USBD_CDC, SDSIO_VCOM_USB_CDC_INSTANCE, _ACM_SetLineCoding) (const CDC_LINE_CODING *line_coding) {
  cdc_acm_line_coding = *line_coding;
  return true;
}

// Called upon USB Host request to retrieve communication settings.
// bool USBD_CDCn_ACM_GetLineCoding (CDC_LINE_CODING *line_coding)
bool CREATE_SYMBOL(USBD_CDC, SDSIO_VCOM_USB_CDC_INSTANCE, _ACM_GetLineCoding) (CDC_LINE_CODING *line_coding) {
  *line_coding = cdc_acm_line_coding;
  return true;
}

// Called when all data was sent on Bulk IN Endpoint.
// void USBD_CDCn_ACM_DataSent (void)
void CREATE_SYMBOL(USBD_CDC, SDSIO_VCOM_USB_CDC_INSTANCE, _ACM_DataSent) (void) {
  osEventFlagsSet(sdsioEventFlagId, SDSIO_CLIENT_EVENT_DATA_SENT);
}

// Called when new data is received on Bulk OUT Endpoint.
// void USBD_CDCn_ACM_DataReceived (uint32_t len)
void CREATE_SYMBOL(USBD_CDC, SDSIO_VCOM_USB_CDC_INSTANCE, _ACM_DataReceived) (uint32_t len) {
  osEventFlagsSet(sdsioEventFlagId, SDSIO_CLIENT_EVENT_DATA_RECEIVED);
}

/**
  \fn          int32_t sdsioClientInit (void)
  \brief       Initialize SDS I/O Client via USB Virtual COM Port
  \return      SDSIO_OK on success or
               a negative value on error (see \ref SDS_IO_Return_Codes)
*/
int32_t sdsioClientInit (void) {
  int32_t  ret = SDSIO_ERROR;
  uint32_t expirationTick;

  sdsioEventFlagId = osEventFlagsNew(NULL);
  if (sdsioEventFlagId != NULL) {
    // Initialize USB Device stack.
    ret = SDSIO_ERROR_INTERFACE;
    expirationTick = osKernelGetTickCount() + SDSIO_VCOM_TIMEOUT;
    if (USBD_Initialize(SDSIO_VCOM_USB_DEVICE_INDEX) == usbOK) {
      if (USBD_Connect(SDSIO_VCOM_USB_DEVICE_INDEX) == usbOK) {
        while (osKernelGetTickCount() < expirationTick) {
          if (USBD_Configured(SDSIO_VCOM_USB_DEVICE_INDEX) == true) {
            ret = SDSIO_OK;
            break;
          } else {
            osDelay(1);
          }
        }
      }
    }
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
  USBD_Disconnect(SDSIO_VCOM_USB_DEVICE_INDEX);
  USBD_Uninitialize(SDSIO_VCOM_USB_DEVICE_INDEX);
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
  int32_t vcom_status, event_status;

  while (num < buf_size) {
    vcom_status = USBD_CDC_ACM_WriteData(SDSIO_VCOM_USB_CDC_INSTANCE,
                                         buf + num,
                                        (int32_t)(buf_size - num));
    if (vcom_status < 0) {
      // Error happened.
      if (vcom_status == usbTimeout) {
        // Timeout happened.
        ret = SDSIO_ERROR_TIMEOUT;
      } else {
        // Error happened.
        ret = SDSIO_ERROR_INTERFACE;
      }
      break;
    }
    num += (uint32_t)vcom_status;
    if (num < buf_size) {
      // Wait for data sent event.
      event_status = osEventFlagsWait(sdsioEventFlagId,
                                      SDSIO_CLIENT_EVENT_DATA_SENT,
                                      osFlagsWaitAll,
                                      SDSIO_VCOM_TIMEOUT);
      if ((event_status & osFlagsError) != 0U) {
        if (event_status == osErrorTimeout) {
          // Timeout happened.
          ret = SDSIO_ERROR_TIMEOUT;
        } else {
          // Error happened.
          ret = SDSIO_ERROR;
        }
        break;
      }
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
  int32_t vcom_status, event_status;

  while (num < buf_size) {
    vcom_status = USBD_CDC_ACM_ReadData(SDSIO_VCOM_USB_CDC_INSTANCE,
                                    buf + num,
                                    (int32_t)(buf_size - num));
    if (vcom_status < 0) {
      // Error happened.
      if (vcom_status == usbTimeout) {
        // Timeout happened.
        ret = SDSIO_ERROR_TIMEOUT;
      } else {
        // Error happened.
        ret = SDSIO_ERROR_INTERFACE;
      }
      break;
    }
    num += (uint32_t)vcom_status;
    if (num < buf_size) {
      // Wait for data received event.
      event_status = osEventFlagsWait(sdsioEventFlagId,
                                      SDSIO_CLIENT_EVENT_DATA_RECEIVED,
                                      osFlagsWaitAll,
                                      SDSIO_VCOM_TIMEOUT);
      if ((event_status & osFlagsError) != 0U) {
        if (event_status == osErrorTimeout) {
          // Timeout happened.
          ret = SDSIO_ERROR_TIMEOUT;
        } else {
          // Error happened.
          ret = SDSIO_ERROR;
        }
        break;
      }
    }
  }

  if (num != 0U) {
    ret = num;
  }
  return ret;
}
