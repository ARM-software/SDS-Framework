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

#ifndef ISM330DHCX_FIFO_H
#define ISM330DHCX_FIFO_H

#ifdef  __cplusplus
extern "C"
{
#endif

#include <stdint.h>

#define ISM330DHCX_ID_GYROSCOPE      0U
#define ISM330DHCX_ID_ACCELEROMETER  1U

#define ISM330DHCX_TAG(id)           (id + 1U)

/**
  \fn          int32_t ISM330DHCX_FIFO_Init (uint32_t id)
  \brief       Initialize FIFO.
  \param[in]   id          ISM330DHCX_ID_GYROSCOPE or ISM330DHCX_ID_ACCELEROMETER
  \return      0=Ok, -1=Error
*/
int32_t ISM330DHCX_FIFO_Init (uint32_t id);

/**
  \fn          int32_t ISM330DHCX_FIFO_Uninit (uint32_t id)
  \brief       Uninitialize FIFO.
  \param[in]   id          ISM330DHCX_ID_GYROSCOPE or ISM330DHCX_ID_ACCELEROMETER
  \return      0=Ok, -1=Error
*/
int32_t ISM330DHCX_FIFO_Uninit (uint32_t id);

/**
  \fn          uint32_t ISM330DHCX_FIFO_Read (uint32_t id, uint32_t num_samples, uint8_t *buf)
  \brief       Read samples from ISM330DHCX (FIFO)
  \param[in]   id          ISM330DHCX_ID_GYROSCOPE or ISM330DHCX_ID_ACCELEROMETER
  \param[in]   num_samples maximum number of samples to read
  \param[out]  buf         pointer to buffer for samples
  \return      number of samples read
*/
uint32_t ISM330DHCX_FIFO_Read (uint32_t id, uint32_t num_samples, uint8_t *buf);

#endif  /* ISM330DHCX_FIFO_H */
