/*---------------------------------------------------------------------------
 * Copyright (c) 2021-2022 Arm Limited (or its affiliates).
 * All rights reserved.
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
 *---------------------------------------------------------------------------*/

#include "audio_drv.h"

#include "fsl_sai_edma.h"
#include "fsl_dmamux.h"

/* Config: SAI */
#define SAI_INSTANCE          SAI1
#define SAI_DATA_ORDER        kSAI_DataMSB    /* LSB or MSB transmitted first       */
#define SAI_SYNC_MODE_TX      kSAI_ModeAsync  /* Synchronous or Asynchronous        */
#define SAI_SYNC_MODE_RX      kSAI_ModeSync   /* Synchronous or Asynchronous        */
#define SAI_MODE_MASTERSLAVE  kSAI_Master     /* Master or Slave (BCLK, frame sync) */
#define SAI_MODE_MONO         kSAI_Stereo     /* Left or Right Channel              */

/* Config: DMA for SAI */
#define SAI_DMA_ENABLE_TX     1U
#define SAI_DMA_CHANNEL_TX    0U
#define SAI_DMA_SOURCE_TX     kDmaRequestMuxSai1Tx

#define SAI_DMA_ENABLE_RX     1U
#define SAI_DMA_CHANNEL_RX    1U
#define SAI_DMA_SOURCE_RX     kDmaRequestMuxSai1Rx
