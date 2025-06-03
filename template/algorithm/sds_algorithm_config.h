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

#ifndef SDS_ALGORITHM_CONFIG_H_
#define SDS_ALGORITHM_CONFIG_H_

// Input Data block size, in bytes
#ifndef SDS_ALGO_DATA_IN_BLOCK_SIZE
#warning "Configure algorithm under test input data block size (SDS_ALGO_DATA_IN_BLOCK_SIZE) for your algorithm and remove this line!"
#define SDS_ALGO_DATA_IN_BLOCK_SIZE     (1024)
#endif

// Output Data block size, in bytes
#ifndef SDS_ALGO_DATA_OUT_BLOCK_SIZE
#warning "Configure algorithm under test output data block size (SDS_ALGO_DATA_OUT_BLOCK_SIZE) for your algorithm and remove this line!"
#define SDS_ALGO_DATA_OUT_BLOCK_SIZE    (32)
#endif

#endif
