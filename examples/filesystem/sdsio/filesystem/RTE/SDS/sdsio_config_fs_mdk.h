/*
 * Copyright (c) 2023 Arm Limited. All rights reserved.
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
 * Name:    sdsio_config_fs_mdk.h
 * Purpose: SDS IO via File System (Keil::File System) configuration options
 * Rev.:    V1.0.0
 */

//-------- <<< Use Configuration Wizard in Context Menu >>> --------------------

// <h>SDS IO via File System (Keil::File System)

//   <s.32>Drive
//   <i>File System drive
//   <i>Default: "M0:"
#define SDSIO_DRIVE                 "M0:"

//   <s.128>Working directory
//   <i>- If the drive prefix is omitted, the current drive is used.
//   <i>- Path string must end with delimiter character '\\' or '/'.
//   <i>Default: ""
#define SDSIO_WORK_DIR              ""

//   <o.0>Formatting allowed
//   <i>Allow formatting, when there is no file system on the drive.
//   <i>Default: 1
#define SDSIO_FORMATTING_ALLOWED    1U

// </h>

//------------- <<< end of configuration section >>> ---------------------------
