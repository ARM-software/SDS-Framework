/*---------------------------------------------------------------------------
 * Copyright (c) 2021-2023 Arm Limited (or its affiliates).
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

#include <string.h>
#include <stdlib.h>
#include "audio_drv.h"
#include "b_u585i_iot02a_audio.h"

/* Definitions */
typedef struct audio_cb_s {
  AudioDrv_Event_t callback;
  uint8_t *app_buf;
  uint8_t *dma_buf;
  uint32_t block_num;
  uint32_t block_size;
  uint32_t block_idx;
  uint32_t count;
  uint8_t  initialized;
  uint8_t  locked;
  uint16_t reserved;
} AUDIO_CB;

static int32_t err_to_audio (int32_t bsp_err);
static void    free_buf (void);
