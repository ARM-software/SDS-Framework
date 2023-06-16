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

// Temperature
#define SENSOR2_NAME            "Temperature"
#define SENSOR2_DMA_MODE        0U
#define SENSOR2_SAMPLE_SIZE     4U
#define SENSOR2_SAMPLE_INTERVAL 1000000U
#define SENSOR2_FIFO_SIZE       4U
#define SENSOR2_DATA_THRESHOLD  0U

// Humidity
#define SENSOR3_NAME            "Humidity"
#define SENSOR3_DMA_MODE        0U
#define SENSOR3_SAMPLE_SIZE     4U
#define SENSOR3_SAMPLE_INTERVAL 1000000U
#define SENSOR3_FIFO_SIZE       4U
#define SENSOR3_DATA_THRESHOLD  0U

// Pressure
#define SENSOR4_NAME            "Pressure"
#define SENSOR4_DMA_MODE        0U
#define SENSOR4_SAMPLE_SIZE     4U
#define SENSOR4_SAMPLE_INTERVAL 25000U
#define SENSOR4_FIFO_SIZE       4U
#define SENSOR4_DATA_THRESHOLD  0U

// Accelerometer
#define SENSOR5_NAME            "Accelerometer"
#define SENSOR5_DMA_MODE        0U
#define SENSOR5_SAMPLE_SIZE     6U
#define SENSOR5_SAMPLE_INTERVAL 600U
#define SENSOR5_FIFO_SIZE       1536U
#define SENSOR5_DATA_THRESHOLD  0U

// Gyroscope
#define SENSOR6_NAME            "Gyroscope"
#define SENSOR6_DMA_MODE        0U
#define SENSOR6_SAMPLE_SIZE     6U
#define SENSOR6_SAMPLE_INTERVAL 600U
#define SENSOR6_FIFO_SIZE       1536U
#define SENSOR6_DATA_THRESHOLD  0U

// Magnetometer
#define SENSOR7_NAME            "Magnetometer"
#define SENSOR7_DMA_MODE        0U
#define SENSOR7_SAMPLE_SIZE     6U
#define SENSOR7_SAMPLE_INTERVAL 10000U
#define SENSOR7_FIFO_SIZE       6U
#define SENSOR7_DATA_THRESHOLD  0U

#endif /* SENSOR_CONFIG_H */
