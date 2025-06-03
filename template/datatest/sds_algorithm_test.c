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

#include "sds_algorithm_config.h"
#include "sds_algorithm.h"


/**
  \fn           int32_t InitAlgorithm (void)
  \brief        Initialize algorithm under test.
  \return       0 on success; -1 on error
*/
int32_t InitAlgorithm (void) {

  // No initialization necessary

  return 0;
}

/**
  \fn           int32_t ExecuteAlgorithm (const uint8_t *in_buf, uint32_t in_num, uint8_t *out_buf, uint32_t out_num)
  \brief        Execute algorithm under test.
  \param[in]    in_buf          pointer to memory buffer containing input data for algorithm
  \param[in]    in_num          number of data bytes in input data buffer (in bytes)
  \param[out]   out_buf         pointer to memory buffer for returning algorithm output
  \param[in]    out_num         maximum number of data bytes returned as algorithm output (in bytes)
  \return       0 on success; -1 on error
*/
int32_t ExecuteAlgorithm (const uint8_t *in_buf, uint32_t in_num, uint8_t *out_buf, uint32_t out_num) {
  uint32_t  sum_x, sum_y, sum_z;
  uint16_t *ptr_axis;
  uint16_t *ptr_out;
  int32_t   i;

  // Process input data
  ptr_axis = (uint16_t *)in_buf;
  sum_x = sum_y = sum_z = 0U;
  for (i = 0; i < (in_num / 6); i++) {
    sum_x    += *ptr_axis;
    ptr_axis += 1;
    sum_y    += *ptr_axis;
    ptr_axis += 1;
    sum_z    += *ptr_axis;
    ptr_axis += 1;
  }
  sum_x = (sum_x & 0xFFFF) + (sum_x >> 16);
  sum_y = (sum_y & 0xFFFF) + (sum_y >> 16);
  sum_z = (sum_z & 0xFFFF) + (sum_z >> 16);

  // Store output data
  ptr_out = (uint16_t *)out_buf;
  for (i = 0; i < (out_num / 4); i++) {
    *ptr_out = (sum_x ^ sum_z) & 0xFFFF;
    ptr_out += 1;
    *ptr_out = (sum_y ^ sum_z) & 0xFFFF;
    ptr_out += 1;
    sum_z += 12345U;
  }

  return 0;
}
