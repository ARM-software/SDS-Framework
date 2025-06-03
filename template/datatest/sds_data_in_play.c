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

#include "cmsis_os2.h"
#include "sds_main.h"
#include "sds_control.h"
#include "sds_rec_play.h"
#include "sds_algorithm_config.h"
#include "sds_data_in.h"


// Recorded data timestamp
uint32_t playTimestamp = 0U;


/**
  \fn           int32_t InitInputData (void)
  \brief        Initialize system for acquiring input data.
  \return       0 on success; -1 on error
*/
int32_t InitInputData (void) {

  // Playback stream is opened in function OpenStreams (sds_main.c)

  return 0;
}

/**
  \fn           int32_t GetInputData (uint8_t *buf, uint32_t max_len)
  \brief        Get input data block as required for algorithm under test.
  \details      Size of this block has to match size expected by algorithm under test.
  \param[out]   buf             pointer to memory buffer for acquiring input data
  \param[in]    max_len         maximum number of bytes of input data to acquire
  \return       number of data bytes returned; -1 on error
*/
int32_t GetInputData (uint8_t *buf, uint32_t max_len) {
  int32_t retv;

  // Check input parameters
  if ((buf == NULL) || (max_len == 0U)) {
    return -1;
  }

  // Check if buffer can fit expected data
  if (max_len < SDS_ALGO_DATA_IN_BLOCK_SIZE) {
    return -1;
  }

  // Wait for playback activation
  while (sdsStreamingState != SDS_STREAMING_ACTIVE) {
    osDelay(100U);
  }

  // Short delay to safeguard against playback data drainage
  osDelay(10U);

  // Read input data from playback stream
  retv = sdsPlayRead(playIdDataInput, &playTimestamp, buf, max_len);
  if (retv > 0) {
    SDS_ASSERT(retv == max_len);
  } else {
    sdsStreamingState = SDS_STREAMING_STOP;
    retv = -1;
  }

  return retv;
}
