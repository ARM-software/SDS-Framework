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

#include "main.h"
#include "cmsis_vio.h"
#include "cmsis_os2.h"

#include "sds_rec.h"

#include "sensor_drv.h"
#include "sensor_config.h"

// Configuration
#ifndef REC_BUF_SIZE_ACCELEROMETER
#define REC_BUF_SIZE_ACCELEROMETER          8192U
#endif
#ifndef REC_BUF_SIZE_TEMPERATURE_SENSOR
#define REC_BUF_SIZE_TEMPERATURE_SENSOR     256U
#endif
#ifndef REC_IO_THRESHOLD_ACCELEROMETER
#define REC_IO_THRESHOLD_ACCELEROMETER      512
#endif
#ifndef REC_IO_THRESHOLD_TEMPERATURE_SENSOR
#define REC_IO_THRESHOLD_TEMPERATURE_SENSOR 0
#endif

#ifndef SENSOR_POLLING_INTERVAL
#define SENSOR_POLLING_INTERVAL             50U  // 50ms
#endif

#ifndef SENSOR_BUF_SIZE
#define SENSOR_BUF_SIZE                     8192U
#endif

// Sensor identifiers
static sensorId_t sensorId_accelerometer;
static sensorId_t sensorId_temperatureSensor;

// Sensor configuration
static sensorConfig_t *sensorConfig_accelerometer;
static sensorConfig_t *sensorConfig_temperatureSensor;

// Recorder identifiers
static sdsRecId_t recId_accelerometer     = NULL;
static sdsRecId_t recId_temperatureSensor = NULL;

// Recorder buffers
static uint8_t recBuf_accelerometer[REC_BUF_SIZE_ACCELEROMETER];
static uint8_t recBuf_temperatureSensor[REC_BUF_SIZE_TEMPERATURE_SENSOR];

// Temporary sensor buffer
static uint8_t sensorBuf[SENSOR_BUF_SIZE];

// Sensor close flag
static uint8_t close_flag = 0U;

// Event close sent flag
static uint8_t event_close_sent;

// Thread identifiers
static osThreadId_t thrId_demo;

#define EVENT_CLOSE     (1U << 0)

// Read sensor thread
static __NO_RETURN void read_sensors (void *argument) {
  uint32_t num, buf_size;
  uint32_t timestamp;
  (void)   argument;

  timestamp = osKernelGetTickCount();
  for (;;) {
    if (close_flag == 0U) {
      if (sensorGetStatus(sensorId_accelerometer).active != 0U) {
        num = sizeof(sensorBuf) / sensorConfig_accelerometer->sample_size;
        num = sensorReadSamples(sensorId_accelerometer, num, sensorBuf, sizeof(sensorBuf));
        if (num != 0U) {
          buf_size = num * sensorConfig_accelerometer->sample_size;
          num = sdsRecWrite(recId_accelerometer, timestamp, sensorBuf, buf_size);
          if (num != buf_size) {
            printf("%s: Recorder write failed\r\n", sensorConfig_accelerometer->name);
          }
        }
      }

      if (sensorGetStatus(sensorId_temperatureSensor).active != 0U) {
        num = sizeof(sensorBuf) / sensorConfig_temperatureSensor->sample_size;
        num = sensorReadSamples(sensorId_temperatureSensor, num, sensorBuf, sizeof(sensorBuf));
        if (num != 0U) {
          buf_size = num * sensorConfig_temperatureSensor->sample_size;
          num = sdsRecWrite(recId_temperatureSensor, timestamp, sensorBuf, buf_size);
          if (num != buf_size) {
            printf("%s: Recorder write failed\r\n", sensorConfig_temperatureSensor->name);
          }
        }
      }
    } else {
      if (event_close_sent == 0U) {
        event_close_sent = 1U;
        osThreadFlagsSet(thrId_demo, EVENT_CLOSE);
      }
    }

    timestamp += SENSOR_POLLING_INTERVAL;
    osDelayUntil(timestamp);
  }
}

// Recorder event callback
static void recorder_event_callback (sdsRecId_t id, uint32_t event) {
  if (event & SDS_REC_EVENT_IO_ERROR) {
    if (id == recId_accelerometer) {
      printf("%s: Recorder event - I/O error\r\n", sensorConfig_accelerometer->name);
    }
    if (id == recId_temperatureSensor) {
      printf("%s: Recorder event - I/O error\r\n", sensorConfig_temperatureSensor->name);
    }
  }
}

// Recorder start
static void recorder_start (void) {

  // Accelerometer
  // Open Recorder
  recId_accelerometer = sdsRecOpen("Accelerometer",
                                    recBuf_accelerometer,
                                    sizeof(recBuf_accelerometer),
                                    REC_IO_THRESHOLD_ACCELEROMETER);
  sensorEnable(sensorId_accelerometer);
  printf("Accelerometer enabled\r\n");

  // Temperature sensor
  // Open Recorder
  recId_temperatureSensor = sdsRecOpen("Temperature",
                                        recBuf_temperatureSensor,
                                        sizeof(recBuf_temperatureSensor),
                                        REC_IO_THRESHOLD_TEMPERATURE_SENSOR);
  sensorEnable(sensorId_temperatureSensor);
  printf("Temperature sensor enabled\r\n");
}

// Recorder stop
static void recorder_stop (void) {
  uint32_t flags;

  event_close_sent = 0U;
  close_flag = 1U;
  flags = osThreadFlagsWait(EVENT_CLOSE, osFlagsWaitAny, osWaitForever);
  if ((flags & osFlagsError) == 0U) {

    // Accelerometer disable
    sensorDisable(sensorId_accelerometer);
    // Close Recorder
    sdsRecClose(recId_accelerometer);
    recId_accelerometer = NULL;
    printf("Accelerometer disabled\r\n");

    // Temperature sensor disable
    sensorDisable(sensorId_temperatureSensor);
    // Close Recorder
    sdsRecClose(recId_temperatureSensor);
    recId_temperatureSensor = NULL;
    printf("Temperature sensor disabled\r\n");
  }

  close_flag = 0U;
}

// Demo
static __NO_RETURN void demo (void *argument) {
  uint32_t  value;
  uint32_t  value_last = 0U;
  uint8_t   rec_active = 0U;

  (void) argument;

  thrId_demo = osThreadGetId();

  // Get sensor identifier
  sensorId_accelerometer     = sensorGetId("Accelerometer");
  sensorId_temperatureSensor = sensorGetId("Temperature");

  // Get sensor configuration
  sensorConfig_accelerometer     = sensorGetConfig(sensorId_accelerometer);
  sensorConfig_temperatureSensor = sensorGetConfig(sensorId_temperatureSensor);

  // Initialize recorder
  sdsRecInit(recorder_event_callback);

  // Create sensor thread
  osThreadNew(read_sensors, NULL, NULL);

  for(;;) {
    // Monitor user button
    value = vioGetSignal(vioBUTTON0);
    if (value != value_last) {
      value_last = value;
      if (value == vioBUTTON0) {
        // Button pressed
        if (rec_active == 0U) {
          rec_active = 1U;
          recorder_start();
        } else {
          rec_active = 0U;
          recorder_stop();
        }
      }
    }
    osDelay(100U);
  }
}

// Application initialization
int32_t app_initialize (void) {
  osThreadNew(demo, NULL, NULL);
  return 0;
}
