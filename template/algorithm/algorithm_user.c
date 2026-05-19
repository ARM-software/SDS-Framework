/*
 * Copyright (c) 2025-2026 Arm Limited. All rights reserved.
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

#include "algorithm_config.h"
#include "algorithm.h"


/**
  \fn           int32_t InitAlgorithm (void)
  \brief        Initialize algorithm under test.
  \return       0 on success; -1 on error
*/
int32_t InitAlgorithm (void) {

#warning "Add initialization code for the algorithm under test."

  return 0;
}

/**
  \fn           void ResetAlgorithm (void)
  \brief        Reset algorithm under test before starting a playback run.
*/
void ResetAlgorithm (void) {
#warning "Add reset logic for the algorithm under test."
}

/**
  \fn           int32_t ExecuteAlgorithm (uint8_t *in_buf, uint32_t in_num, uint8_t *out_buf, uint32_t out_num)
  \brief        Execute algorithm under test.
  \param[in]    in_buf          pointer to memory buffer containing input data for algorithm
  \param[in]    in_num          number of data bytes in input data buffer (in bytes)
  \param[out]   out_buf         pointer to memory buffer for returning algorithm output
  \param[in]    out_num         maximum number of data bytes returned as algorithm output (in bytes)
  \return       0 on success; -1 on error
*/
int32_t ExecuteAlgorithm (uint8_t *in_buf, uint32_t in_num, uint8_t *out_buf, uint32_t out_num) {

#warning "Add implementation code to execute the algorithm under test."

  return 0;
}
