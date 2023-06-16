/*---------------------------------------------------------------------------
 * Copyright (c) 2021-2022 Arm Limited (or its affiliates).
 * All rights reserved.
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
 *---------------------------------------------------------------------------*/

// Define I2C instance used to control audio codec device
#define WM8960_INSTANCE_I2C       1U

// Define I2C address used to address audio codec device
#define WM8960_ADDRESS_I2C        WM8960_I2C_ADDR

// Define codec device operation mode
#define WM8960_MODE_MASTER        false

// Define audio data transfer protocol
#define WM8960_BUS_PROTOCOL       kWM8960_BusI2S

// Define audio data route
#define WM8960_DATA_ROUTE         kWM8960_RoutePlaybackandRecord

// Define left input source
#define WM8960_INPUT_LEFT         kWM8960_InputDifferentialMicInput3

// Define right input source
#define WM8960_INPUT_RIGHT        kWM8960_InputDifferentialMicInput2

// Define input source for left and right mixer (i.e. output source to speaker)
#define WM8960_MIXER_SOURCE       kWM8960_PlaySourceDAC
