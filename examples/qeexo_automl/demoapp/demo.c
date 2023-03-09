/*
 * Copyright (c) 2022-2023 Arm Limited. All rights reserved.
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

#include <stdio.h>
#include <string.h>

#include "QxAutoMLUser.h"

#include "cmsis_vio.h"
#include "cmsis_os2.h"

#include "sensor_drv.h"
#include "sensor_config.h"

// Configuration
#ifndef SENSOR_POLLING_INTERVAL
#define SENSOR_POLLING_INTERVAL             50U  /* 50ms */
#endif

#ifndef SENSOR_BUF_SIZE
#define SENSOR_BUF_SIZE                     1000U
#endif

// Sensor identifiers
static sensorId_t sensorId_accelerometer              = NULL;
static sensorId_t sensorId_gyroscope                  = NULL;

// Sensor configuration
static sensorConfig_t *sensorConfig_accelerometer     = NULL;
static sensorConfig_t *sensorConfig_gyroscope         = NULL;

// Temporary sensor buffer
static uint8_t sensorBuf[SENSOR_BUF_SIZE];

// Structured Sensor Buffer Data
typedef struct {
  int16_t x;
  int16_t y;
  int16_t z;

} SENSOR_AxesRaw_t;

static uint8_t sensorAxesBuf[SENSOR_BUF_SIZE];

// Sensor close flag
static uint8_t close_flag = 0U;

// Thread identifiers
static osThreadId_t thrId_demo           = NULL;
static osThreadId_t thrId_read_sensors   = NULL;

// Read sensor thread
static __NO_RETURN void read_sensors (void *argument) {
  uint32_t num, buf_size;
  uint32_t timestamp;
  (void)   argument;

  timestamp = osKernelGetTickCount();
  for (;;) {

    // Collect sensor data.
    if (sensorGetStatus(sensorId_accelerometer).active != 0U) {
      num = sizeof(sensorBuf) / sensorConfig_accelerometer->sample_size;
      num = sensorReadSamples(sensorId_accelerometer, num, sensorBuf, sizeof(sensorBuf));
      if ((num != 0U) && (num < SENSOR_BUF_SIZE)) {
        memcpy(sensorAxesBuf, sensorBuf, sensorConfig_accelerometer->sample_size * num);
        QxFillSensorData(QXSENSOR_TYPE_ACCEL, sensorAxesBuf, num * sensorConfig_accelerometer->sample_size);
      }
    }

    if (sensorGetStatus(sensorId_gyroscope).active != 0U) {
      num = sizeof(sensorBuf) / sensorConfig_gyroscope->sample_size;
      num = sensorReadSamples(sensorId_gyroscope, num, sensorBuf, sizeof(sensorBuf));
      if ((num != 0U) && (num < SENSOR_BUF_SIZE)) {
        memcpy(sensorAxesBuf, sensorBuf, sensorConfig_gyroscope->sample_size * num);
        QxFillSensorData(QXSENSOR_TYPE_GYRO, sensorAxesBuf, num * sensorConfig_gyroscope->sample_size);
      }
    }

    timestamp += SENSOR_POLLING_INTERVAL;
    osDelayUntil(timestamp);
  }
}

const char *qx_predict_classes[] = {"REST", "SHAKE", "WAVE"};

typedef struct {
	uint8_t reserved[10];
	float mProbs[50];
	}	tPrepIntDataFrame;

extern tPrepIntDataFrame predFrame;
	
	
#include <stdarg.h>

// Sensor Demo
void __NO_RETURN demo(void) {
  //uint32_t  n, num, flags;
  //uint32_t  buf[2];
  //uint16_t *data_u16 = (uint16_t *)buf;
  //float    *data_f   = (float *)buf;
	uint32_t inference_interval=0;
	int ClassificationResult;
	
  thrId_demo = osThreadGetId();

  // Get sensor identifier
  sensorId_accelerometer     = sensorGetId("Accelerometer");
  sensorId_gyroscope         = sensorGetId("Gyroscope");

  // Get sensor configuration
  sensorConfig_accelerometer     = sensorGetConfig(sensorId_accelerometer);
  sensorConfig_gyroscope         = sensorGetConfig(sensorId_gyroscope);

	  // Create sensor thread
  thrId_read_sensors = osThreadNew(read_sensors, NULL, NULL);

  sensorEnable(sensorId_accelerometer);
  printf("Accelerometer enabled\r\n");
   
  sensorEnable(sensorId_gyroscope);
  printf("Gyroscope enabled\r\n");

  while(1) {
	  
		inference_interval = osKernelGetTickCount() + PRED_CLASSIFICATION_INTERVAL_IN_MSECS;
		ClassificationResult = QxClassify();
		printf("TS: %d Prediction: %d - %s\r\n", inference_interval, ClassificationResult, qx_predict_classes[ClassificationResult]);
    osDelayUntil(inference_interval);		    
  }
}
