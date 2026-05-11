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

#ifndef ALGORITHM_CONFIG_H_
#define ALGORITHM_CONFIG_H_

// User configured test bandwidth in bytes/sec
#ifndef ALGO_TEST_BANDWIDTH
#define ALGO_TEST_BANDWIDTH             100000U
#endif

// User configured test interval in ms
#ifndef ALGO_TEST_INTERVAL
#define ALGO_TEST_INTERVAL              10U
#endif

// Input Data block size, in bytes
#ifndef ALGO_DATA_IN_BLOCK_SIZE
#define ALGO_DATA_IN_BLOCK_SIZE         ((((ALGO_TEST_BANDWIDTH * ALGO_TEST_INTERVAL) / 1000) / 6) * 6)
#endif

// Output Data block size, in bytes
#ifndef ALGO_DATA_OUT_BLOCK_SIZE
#define ALGO_DATA_OUT_BLOCK_SIZE        (40)
#endif

#endif
