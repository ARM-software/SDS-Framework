/*
 * Copyright (c) 2022 Arm Limited. All rights reserved.
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

#ifndef SENSOR_CONFIG_H
#define SENSOR_CONFIG_H

// Sensor Configuration

// Accelerometer
#define SENSOR3_NAME            "Accelerometer"
#define SENSOR3_DMA_MODE        0U
#define SENSOR3_SAMPLE_SIZE     6U
#define SENSOR3_SAMPLE_INTERVAL 5000U
#define SENSOR3_FIFO_SIZE       32U
#define SENSOR3_DATA_THRESHOLD  0U

// Magnetometer
#define SENSOR5_NAME            "Magnetometer"
#define SENSOR5_DMA_MODE        0U
#define SENSOR5_SAMPLE_SIZE     6U
#define SENSOR5_SAMPLE_INTERVAL 5000U
#define SENSOR5_FIFO_SIZE       0U
#define SENSOR5_DATA_THRESHOLD  0U

#endif /* SENSOR_CONFIG_H */
