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
 *
 * Name:    sdsio_config_usb_mdk.h
 * Purpose: SDS IO via USB (Keil::USB:Device:Custom) configuration options
 * Rev.:    V2.0.0
 */

//-------- <<< Use Configuration Wizard in Context Menu >>> --------------------

// <h>SDS IO via USB (Keil::USB:Device:Custom Class)

//   <o>USB Device index <0-3>
//   <i>Index of USB Device
//   <i>Default: 0
#define SDSIO_USB_DEVICE_INDEX          0

//   <o>USB Device Custom Class instance <0-3>
//   <i>Index of Custom Class
//   <i>Default: 0
#define SDSIO_USB_INSTANCE              0

//   <o>Transfer timeout
//   <i>Send and receive timeout in kernel ticks
//   <i>Default: 3000
#define SDSIO_USB_TIMEOUT               3000U

//   <o>USB Bulk OUT Buffer Size
//   <i>Size of the temporary USB Bulk OUT buffer in bytes
//   <i>Must be a multiple of MAX_PACKET size (n * 64 for FS, n * 512 for HS)
//   <i>Default: 8192
#define SDSIO_USB_BULK_OUT_BUF_SIZE     8192U

// </h>

//------------- <<< end of configuration section >>> ---------------------------
