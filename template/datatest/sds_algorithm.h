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

#ifndef SDS_ALGORITHM_H_
#define SDS_ALGORITHM_H_

#include <stdint.h>

#ifdef  __cplusplus
extern "C"
{
#endif

/**
  \fn           int32_t InitAlgorithm (void)
  \brief        Initialize algorithm under test.
  \return       0 on success; -1 on error
*/
extern int32_t InitAlgorithm (void);

/**
  \fn           int32_t ExecuteAlgorithm (const uint8_t *in_buf, uint32_t in_num, uint8_t *out_buf, uint32_t out_num)
  \brief        Execute algorithm under test.
  \param[in]    in_buf          pointer to memory buffer containing input data for algorithm
  \param[in]    in_num          number of data bytes in input data buffer (in bytes)
  \param[out]   out_buf         pointer to memory buffer for returning algorithm output
  \param[in]    out_num         maximum number of data bytes returned as algorithm output (in bytes)
  \return       0 on success; -1 on error
*/
extern int32_t ExecuteAlgorithm (const uint8_t *in_buf, uint32_t in_num, uint8_t *out_buf, uint32_t out_num);

#ifdef  __cplusplus
}
#endif

#endif
