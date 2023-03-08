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

#include "cmsis_vio.h"
#include "cmsis_os2.h"

#include "sds.h"
#ifdef RECORDER_ENABLED
#include "sds_rec.h"
#endif

#include "sensor_drv.h"
#include "sensor_config.h"

// Configuration
#ifndef SDS_BUF_SIZE_ACCELEROMETER
#define SDS_BUF_SIZE_ACCELEROMETER          8192U
#endif
#ifndef SDS_BUF_SIZE_GYROSCOPE
#define SDS_BUF_SIZE_GYROSCOPE              8192U
#endif
#ifndef SDS_BUF_SIZE_TEMPERATURE_SENSOR
#define SDS_BUF_SIZE_TEMPERATURE_SENSOR     128U
#endif
#ifndef SDS_THRESHOLD_ACCELEROMETER
#define SDS_THRESHOLD_ACCELEROMETER         2048U
#endif
#ifndef SDS_THRESHOLD_GYROSCOPE
#define SDS_THRESHOLD_GYROSCOPE             2048U
#endif
#ifndef SDS_THRESHOLD_TEMPERATURE_SENSOR
#define SDS_THRESHOLD_TEMPERATURE_SENSOR    4U
#endif

#ifdef RECORDER_ENABLED
#ifndef REC_BUF_SIZE_ACCELEROMETER
#define REC_BUF_SIZE_ACCELEROMETER          8192U
#endif
#ifndef REC_BUF_SIZE_GYROSCOPE
#define REC_BUF_SIZE_GYROSCOPE              8192U
#endif
#ifndef REC_BUF_SIZE_TEMPERATURE_SENSOR
#define REC_BUF_SIZE_TEMPERATURE_SENSOR     256U
#endif
#ifndef REC_IO_THRESHOLD_ACCELEROMETER
#define REC_IO_THRESHOLD_ACCELEROMETER      0
#endif
#ifndef REC_IO_THRESHOLD_GYROSCOPE
#define REC_IO_THRESHOLD_GYROSCOPE          0
#endif
#ifndef REC_IO_THRESHOLD_TEMPERATURE_SENSOR
#define REC_IO_THRESHOLD_TEMPERATURE_SENSOR 0
#endif
#endif

#ifndef SENSOR_POLLING_INTERVAL
#define SENSOR_POLLING_INTERVAL             50U  /* 50ms */
#endif

#ifndef SENSOR_BUF_SIZE
#define SENSOR_BUF_SIZE                     8192U
#endif

// Sensor identifiers
static sensorId_t sensorId_accelerometer              = NULL;
static sensorId_t sensorId_gyroscope                  = NULL;
static sensorId_t sensorId_temperatureSensor          = NULL;

// Sensor configuration
static sensorConfig_t *sensorConfig_accelerometer     = NULL;
static sensorConfig_t *sensorConfig_gyroscope         = NULL;
static sensorConfig_t *sensorConfig_temperatureSensor = NULL;

// SDS identifiers
static sdsId_t sdsId_accelerometer                    = NULL;
static sdsId_t sdsId_gyroscope                        = NULL;
static sdsId_t sdsId_temperatureSensor                = NULL;

// SDS buffers
static uint8_t sdsBuf_accelerometer[SDS_BUF_SIZE_ACCELEROMETER];
static uint8_t sdsBuf_gyroscope[SDS_BUF_SIZE_ACCELEROMETER];
static uint8_t sdsBuf_temperatureSensor[SDS_BUF_SIZE_TEMPERATURE_SENSOR];

#ifdef RECORDER_ENABLED
// Recorder identifiers
static sdsRecId_t recId_accelerometer                 = NULL;
static sdsRecId_t recId_gyroscope                     = NULL;
static sdsRecId_t recId_temperatureSensor             = NULL;

// Recorder buffers
static uint8_t recBuf_accelerometer[REC_BUF_SIZE_ACCELEROMETER];
static uint8_t recBuf_gyroscope[REC_BUF_SIZE_GYROSCOPE];
static uint8_t recBuf_temperatureSensor[REC_BUF_SIZE_TEMPERATURE_SENSOR];
#endif

// Temporary sensor buffer
static uint8_t sensorBuf[SENSOR_BUF_SIZE];

// Sensor close flag
static uint8_t close_flag = 0U;

// Event close sent flag
static uint8_t event_close_sent;

// Thread identifiers
static osThreadId_t thrId_demo           = NULL;
static osThreadId_t thrId_read_sensors   = NULL;

#define EVENT_DATA_ACCELEROMETER        (1U << 0)
#define EVENT_DATA_GYROSCOPE            (1U << 1)
#define EVENT_DATA_TEMPERATURE_SENSOR   (1U << 2)
#define EVENT_BUTTON                    (1U << 3)
#define EVENT_DATA_MASK                 (EVENT_DATA_ACCELEROMETER     | \
                                         EVENT_DATA_GYROSCOPE         | \
                                         EVENT_DATA_TEMPERATURE_SENSOR)
#define EVENT_MASK                      (EVENT_DATA_MASK | EVENT_BUTTON)

#define EVENT_CLOSE                     (1U << 0)

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
          num = sdsWrite(sdsId_accelerometer, sensorBuf, buf_size);
          if (num != buf_size) {
            printf("%s: SDS write failed\r\n", sensorConfig_accelerometer->name);
          }
#ifdef RECORDER_ENABLED
          num = sdsRecWrite(recId_accelerometer, timestamp, sensorBuf, buf_size);
          if (num != buf_size) {
            printf("%s: Recorder write failed\r\n", sensorConfig_accelerometer->name);
          }
#endif
        }
      }

      if (sensorGetStatus(sensorId_gyroscope).active != 0U) {
        num = sizeof(sensorBuf) / sensorConfig_gyroscope->sample_size;
        num = sensorReadSamples(sensorId_gyroscope, num, sensorBuf, sizeof(sensorBuf));
        if (num != 0U) {
          buf_size = num * sensorConfig_gyroscope->sample_size;
          num = sdsWrite(sdsId_gyroscope, sensorBuf, buf_size);
          if (num != buf_size) {
            printf("%s: SDS write failed\r\n", sensorConfig_gyroscope->name);
          }
#ifdef RECORDER_ENABLED
          num = sdsRecWrite(recId_gyroscope, timestamp, sensorBuf, buf_size);
          if (num != buf_size) {
            printf("%s: Recorder write failed\r\n", sensorConfig_gyroscope->name);
          }
#endif
        }
      }

      if (sensorGetStatus(sensorId_temperatureSensor).active != 0U) {
        num = sizeof(sensorBuf) / sensorConfig_temperatureSensor->sample_size;
        num = sensorReadSamples(sensorId_temperatureSensor, num, sensorBuf, sizeof(sensorBuf));
        if (num != 0U) {
          buf_size = num * sensorConfig_temperatureSensor->sample_size;
          num = sdsWrite(sdsId_temperatureSensor, sensorBuf, buf_size);
          if (num != buf_size) {
            printf("%s: SDS write failed\r\n", sensorConfig_temperatureSensor->name);
          }
#ifdef RECORDER_ENABLED
          num = sdsRecWrite(recId_temperatureSensor, timestamp, sensorBuf, buf_size);
          if (num != buf_size) {
            printf("%s: Recorder write failed\r\n", sensorConfig_temperatureSensor->name);
          }
#endif
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

// Button thread
static __NO_RETURN void button (void *argument) {
  uint32_t value, value_last = 0U;
  (void)   argument;

  for (;;) {
    // Monitor user button
    value = vioGetSignal(vioBUTTON0);
    if (value != value_last) {
      value_last = value;
      if (value == vioBUTTON0) {
        // Button pressed
        osThreadFlagsSet(thrId_demo, EVENT_BUTTON);
      }
    }
    osDelay(100U);
  }
}

// SDS event callback
static void sds_event_callback (sdsId_t id, uint32_t event, void *arg) {
  (void)arg;

  if ((event & SDS_EVENT_DATA_HIGH) != 0U) {
    if (id == sdsId_accelerometer) {
      osThreadFlagsSet(thrId_demo, EVENT_DATA_ACCELEROMETER);
    }
    if (id == sdsId_gyroscope) {
      osThreadFlagsSet(thrId_demo, EVENT_DATA_GYROSCOPE);
    }
    if (id == sdsId_temperatureSensor) {
      osThreadFlagsSet(thrId_demo, EVENT_DATA_TEMPERATURE_SENSOR);
    }
  }
}

// Recorder event callback
#ifdef RECORDER_ENABLED
static void recorder_event_callback (sdsRecId_t id, uint32_t event) {
  if (event & SDS_REC_EVENT_IO_ERROR) {
    if (id == recId_accelerometer) {
      printf("%s: Recorder event - I/O error\r\n", sensorConfig_accelerometer->name);
    }
    if (id == recId_gyroscope) {
      printf("%s: Recorder event - I/O error\r\n", sensorConfig_gyroscope->name);
    }
    if (id == recId_temperatureSensor) {
      printf("%s: Recorder event - I/O error\r\n", sensorConfig_temperatureSensor->name);
    }
  }
}
#endif

// button_event
static void button_event (void) {
         uint32_t flags;
  static uint8_t  active = 0U;

  if (active == 0U) {
    active = 1U;

    // Accelerometer enable
    sdsClear(sdsId_accelerometer);
#ifdef RECORDER_ENABLED
    // Open Recorder
    recId_accelerometer = sdsRecOpen("Accelerometer",
                                      recBuf_accelerometer,
                                      sizeof(recBuf_accelerometer),
                                      REC_IO_THRESHOLD_ACCELEROMETER);
#endif
    sensorEnable(sensorId_accelerometer);
    printf("Accelerometer enabled\r\n");

    // Gyroscope enable
    sdsClear(sdsId_gyroscope);
#ifdef RECORDER_ENABLED
    // Open Recorder
    recId_gyroscope = sdsRecOpen("Gyroscope",
                                  recBuf_gyroscope,
                                  sizeof(recBuf_gyroscope),
                                  REC_IO_THRESHOLD_GYROSCOPE);
#endif
    sensorEnable(sensorId_gyroscope);
    printf("Gyroscope enabled\r\n");

    // Temperature sensor enable
    sdsClear(sdsId_temperatureSensor);
#ifdef RECORDER_ENABLED
    // Open Recorder;
    recId_temperatureSensor = sdsRecOpen("Temperature",
                                          recBuf_temperatureSensor,
                                          sizeof(recBuf_temperatureSensor),
                                          REC_IO_THRESHOLD_TEMPERATURE_SENSOR);
#endif
    sensorEnable(sensorId_temperatureSensor);
    printf("Temperature sensor enabled\r\n");
  } else {
    event_close_sent = 0U;
    close_flag = 1U;
    flags = osThreadFlagsWait(EVENT_CLOSE, osFlagsWaitAny, osWaitForever);
    if ((flags & osFlagsError) == 0U) {
      active = 0U;

      // Accelerometer disable
      sensorDisable(sensorId_accelerometer);
#ifdef RECORDER_ENABLED
      // Close Recorder
      sdsRecClose(recId_accelerometer);
      recId_accelerometer = NULL;
#endif
      printf("Accelerometer disabled\r\n");

      // Gyroscope disable
      sensorDisable(sensorId_gyroscope);
#ifdef RECORDER_ENABLED
      // Close Recorder
      sdsRecClose(recId_gyroscope);
      recId_gyroscope = NULL;
#endif
      printf("Gyroscope disabled\r\n");

      // Temperature sensor disable
      sensorDisable(sensorId_temperatureSensor);
#ifdef RECORDER_ENABLED
      // Close Recorder
      sdsRecClose(recId_temperatureSensor);
      recId_temperatureSensor = NULL;
#endif
      printf("Temperature sensor disabled\r\n");
    }

    close_flag = 0U;
  }
}

// Sensor Demo
void __NO_RETURN demo(void) {
  uint32_t  n, num, flags;
  uint32_t  buf[2];
  uint16_t *data_u16 = (uint16_t *)buf;
  float    *data_f   = (float *)buf;

  thrId_demo = osThreadGetId();

  // Get sensor identifier
  sensorId_accelerometer     = sensorGetId("Accelerometer");
  sensorId_gyroscope         = sensorGetId("Gyroscope");
  sensorId_temperatureSensor = sensorGetId("Temperature");

  // Get sensor configuration
  sensorConfig_accelerometer     = sensorGetConfig(sensorId_accelerometer);
  sensorConfig_gyroscope         = sensorGetConfig(sensorId_gyroscope);
  sensorConfig_temperatureSensor = sensorGetConfig(sensorId_temperatureSensor);

  // Open SDS
  sdsId_accelerometer     = sdsOpen(sdsBuf_accelerometer,
                                    sizeof(sdsBuf_accelerometer),
                                    0U, SDS_THRESHOLD_ACCELEROMETER);
                                    
  sdsId_gyroscope         = sdsOpen(sdsBuf_gyroscope,
                                    sizeof(sdsBuf_gyroscope),
                                    0U, SDS_THRESHOLD_GYROSCOPE);

  sdsId_temperatureSensor = sdsOpen(sdsBuf_temperatureSensor,
                                    sizeof(sdsBuf_temperatureSensor),
                                    0U, SDS_THRESHOLD_TEMPERATURE_SENSOR);

  // Register SDS events
  sdsRegisterEvents(sdsId_accelerometer,     sds_event_callback, SDS_EVENT_DATA_HIGH, NULL);
  sdsRegisterEvents(sdsId_gyroscope,         sds_event_callback, SDS_EVENT_DATA_HIGH, NULL);
  sdsRegisterEvents(sdsId_temperatureSensor, sds_event_callback, SDS_EVENT_DATA_HIGH, NULL);

#ifdef RECORDER_ENABLED
  // Initialize recorder
  sdsRecInit(recorder_event_callback);
#endif

  // Create sensor thread
  thrId_read_sensors = osThreadNew(read_sensors, NULL, NULL);

  // Create button thread
  osThreadNew(button, NULL, NULL);

  for(;;) {
    flags = osThreadFlagsWait(EVENT_MASK, osFlagsWaitAny, osWaitForever);
    if ((flags & osFlagsError) == 0U) {

      // Button pressed event
      if (flags & EVENT_BUTTON) {
        printf("Button pressed\r\n");
        button_event();
      }

      // Accelerometer data event
      if ((flags & EVENT_DATA_ACCELEROMETER) != 0U) {
        num = sdsRead(sdsId_accelerometer, buf, sensorConfig_accelerometer->sample_size);
        if (num == sensorConfig_accelerometer->sample_size) {
          printf("%s: x=%i, y=%i, z=%i\r\n",sensorConfig_accelerometer->name,
                                            data_u16[0], data_u16[1], data_u16[2]);
        }
        sdsClear(sdsId_accelerometer);
      }

      // Gyroscope data event
      if ((flags & EVENT_DATA_GYROSCOPE) != 0U) {
        num = sdsRead(sdsId_gyroscope, buf, sensorConfig_gyroscope->sample_size);
        if (num == sensorConfig_gyroscope->sample_size) {
          printf("%s: x=%i, y=%i, z=%i\r\n",sensorConfig_gyroscope->name,
                                            data_u16[0], data_u16[1], data_u16[2]);
        }
        sdsClear(sdsId_gyroscope);
      }

      // Temperature sensor data event
      if ((flags & EVENT_DATA_TEMPERATURE_SENSOR) != 0U) {
        for (n = 0U; n < (SDS_THRESHOLD_TEMPERATURE_SENSOR / sensorConfig_temperatureSensor->sample_size); n++) {
          num = sdsRead(sdsId_temperatureSensor, buf, sensorConfig_temperatureSensor->sample_size);
          if (num == sensorConfig_temperatureSensor->sample_size) {
            printf("%s: value=%f\r\n", sensorConfig_temperatureSensor->name, (double)*data_f);
          }
        }
      }
    }
  }
}
