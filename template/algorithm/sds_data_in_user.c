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

#include <stddef.h>
#include "sds_algorithm_config.h"
#include "sds_data_in.h"


/**
  \fn           int32_t InitInputData (void)
  \brief        Initialize system for acquiring input data.
  \return       0 on success; -1 on error
*/
int32_t InitInputData (void) {

#warning "Add user code to initialize data acquisition!"

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

  // Check input parameters
  if ((buf == NULL) || (max_len == 0U)) {
    return -1;
  }

  // Check if buffer can fit expected data
  if (max_len < SDS_ALGO_DATA_IN_BLOCK_SIZE) {
    return -1;
  }

#warning "Add user code to get a block of input data as required by algorithm under test!"

  return SDS_ALGO_DATA_IN_BLOCK_SIZE;
}
