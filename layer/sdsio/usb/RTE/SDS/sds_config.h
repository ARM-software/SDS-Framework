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

//   <o>Maximum number of concurrent streams <1-31>
//   <i>Default: 16
#define SDS_MAX_STREAMS                 16U

//   <o>Size of a internal working buffer used for I/O transfers
//   <i>Default: 8192
#define SDS_BUF_SIZE                    8192U

// </h>

//------------- <<< end of configuration section >>> ---------------------------

// Thread stack size for SDS system thread
#define SDS_THREAD_STACK_SIZE           1024

// Thread priority for SDS system thread
#define SDS_THREAD_PRIORITY             osPriorityNormal

// Timeout value for opening the SDS stream in kernel ticks
#define SDS_OPEN_TIMEOUT                3000U

// Timeout value for closing the SDS stream in kernel ticks
#define SDS_CLOSE_TIMEOUT               3000U

// Efficient transfer size for IO interface read/write operations
// Default: 8192
// Optimize this value for the underlying IO interface (e.g., socket, USART, VCOM, file system)
// to ensure efficient read/write operations and performance
#define SDS_IO_TRANSFER_SIZE            8192U
