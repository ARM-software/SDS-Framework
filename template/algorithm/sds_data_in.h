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

#ifndef SDS_DATA_IN_H_
#define SDS_DATA_IN_H_

#include <stdint.h>

#ifdef  __cplusplus
extern "C"
{
#endif

/**
  \fn           int32_t InitInputData (void)
  \brief        Initialize system for acquiring input data.
  \return       0 on success; -1 on error
*/
extern int32_t InitInputData (void);

/**
  \fn           int32_t GetInputData (uint8_t *buf, uint32_t max_len)
  \brief        Get input data block as required for algorithm under test.
  \details      Size of this block has to match size expected by algorithm under test.
  \param[out]   buf             pointer to memory buffer for acquiring input data
  \param[in]    max_len         maximum number of bytes of input data to acquire
  \return       number of data bytes returned; -1 on error
*/
extern int32_t GetInputData (uint8_t *buf, uint32_t max_len);

#ifdef  __cplusplus
}
#endif

#endif
