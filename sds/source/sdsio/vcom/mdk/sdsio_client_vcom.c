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

#include "sdsio.h"
#include "sdsio_client.h"
#include "sdsio_config_vcom_mdk.h"

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
  \fn          int32_t sdsioClientInit (void)
  \brief       Initialize SDS I/O Client via USB Virtual COM Port
  \return      SDIOS_OK: initialization success
               SDSIO_ERROR: initialization failed
*/
int32_t sdsioClientInit (void) {
  int32_t ret = SDSIO_ERROR;

  if (USBD_Initialize(SDSIO_USB_DEVICE_INDEX) == usbOK) {
    if (USBD_Connect(SDSIO_USB_DEVICE_INDEX) == usbOK) {
      while (USBD_Configured(SDSIO_USB_DEVICE_INDEX) == false);
      ret = SDSIO_OK;
    }
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
  USBD_Disconnect(SDSIO_USB_DEVICE_INDEX);
  USBD_Uninitialize(SDSIO_USB_DEVICE_INDEX);
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
  uint32_t cnt, num;
  int32_t  status;

  if (header == NULL) {
    return 0U;
  }

  // Send header
  cnt = 0U;
  while (cnt < sizeof(header_t)) {
    status = USBD_CDC_ACM_WriteData(SDSIO_USB_DEVICE_INDEX,
                                    (const uint8_t *)header + cnt,
                                    (int32_t)(sizeof(header_t) - cnt));
    if (status >= 0) {
      cnt += (uint32_t)status;
    } else {
      cnt = 0U;
      break;
    }
  }

  // Send data
  num = cnt;
  cnt = 0U;
  if ((num != 0U) && (data != NULL) && (data_size != 0U)) {
    while (cnt < data_size) {
      status = USBD_CDC_ACM_WriteData(SDSIO_USB_DEVICE_INDEX,
                                      (const uint8_t *)data + cnt,
                                      (int32_t)(data_size - cnt));
      if (status >= 0) {
        cnt += (uint32_t)status;
      } else {
        cnt = 0U;
        break;
      }
    }
  }

  num += cnt;

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
  uint32_t cnt, num, size;
  int32_t  status;

  if (header == NULL) {
    return 0U;
  }

  // Receive header
  cnt = 0U;
  while (cnt < sizeof(header_t)) {
    status = USBD_CDC_ACM_ReadData(SDSIO_USB_DEVICE_INDEX,
                                   (uint8_t *)header + cnt,
                                   (int32_t)(sizeof(header_t) - cnt));
    if (status >= 0) {
      cnt += (uint32_t)status;
    } else {
      cnt = 0U;
      break;
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
      status = USBD_CDC_ACM_ReadData(SDSIO_USB_DEVICE_INDEX,
                                     (uint8_t *)data + cnt,
                                     (int32_t)(size - cnt));
      if (status >= 0) {
        cnt += (uint32_t)status;
      } else {
        cnt = 0U;
        break;
      }
    }
  }

  num += cnt;

  return num;
}
