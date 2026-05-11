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
 *
 * Name:    sds_config.h
 * Purpose: SDS configuration options
 * Rev.:    V3.0.0
 */

//-------- <<< Use Configuration Wizard in Context Menu >>> --------------------

// <h>SDS System Configuration

//   <o>Maximum concurrent streams <1-31>
//   <i>Default: 16
#define SDS_MAX_STREAMS                 16U

//   <o>Internal buffer size for I/O transfers
//   <i>Default: 8192
#define SDS_BUF_SIZE                    8192U

// </h>

//------------- <<< end of configuration section >>> ---------------------------

// SDS system thread stack size
#define SDS_THREAD_STACK_SIZE           1024

// SDS system thread priority
#define SDS_THREAD_PRIORITY             osPriorityNormal

// SDS stream open timeout in kernel ticks
#define SDS_OPEN_TIMEOUT                3000U

// SDS stream close timeout in kernel ticks
#define SDS_CLOSE_TIMEOUT               3000U

// Optimal I/O transfer size (read/write)
// Default: 8192
// Select a value appropriate for the underlying I/O interface (e.g., socket, USART, VCOM, file system)
// to ensure efficient read/write performance
#define SDS_IO_TRANSFER_SIZE            8192U
