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
 * Name:    sdsio_config_serial_usart.h
 * Purpose: SDS IO via Serial Port (CMSIS Driver:USART) configuration options
 * Rev.:    V0.9.0
 */

//-------- <<< Use Configuration Wizard in Context Menu >>> --------------------

// <h>SDS IO via Serial Port (CMSIS Driver:USART)

//   <o>USART driver number (Driver_USART#) <0-255>
//   <i>Exported USART driver control block number (Driver_USART#)
//   <i>Default: 0
#define SDSIO_USART_DRIVER_NUMBER 0

// <h>USART configuration
//   <o>Baudrate
//   <i>Default: 115200
#define SDSIO_USART_BAUDRATE      115200U

//   <o>Data bits <0x500U=>ARM_USART_DATA_BITS_5
//                <0x600U=>ARM_USART_DATA_BITS_6
//                <0x700U=>ARM_USART_DATA_BITS_7
//                <0x000U=>ARM_USART_DATA_BITS_8
//                <0x100U=>ARM_USART_DATA_BITS_9
//   <i>Default: ARM_USART_DATA_BITS_8
#define SDSIO_USART_DATA_BITS     0U

//   <o>Parity <0x0000U=>ARM_USART_PARITY_NONE
//             <0x1000U=>ARM_USART_PARITY_EVEN
//             <0x2000U=>ARM_USART_PARITY_ODD
//   <i>Default: ARM_USART_PARITY_NONE
#define SDSIO_USART_PARITY        0U

//   <o>Stop bits <0x0000U=>ARM_USART_STOP_BITS_1
//                <0x4000U=>ARM_USART_STOP_BITS_2
//                <0x8000U=>ARM_USART_STOP_BITS_1_5
//                <0xC000U=>ARM_USART_STOP_BITS_0_5
//   <i>Default: ARM_USART_STOP_BITS_1
#define SDSIO_USART_STOP_BITS     0U
// </h>

//   <o>USART timeout
//   <i>Send and receive timeout
//   <i>Default: 3000
#define SDSIO_USART_TIMEOUT       3000U

// </h>

//------------- <<< end of configuration section >>> ---------------------------
