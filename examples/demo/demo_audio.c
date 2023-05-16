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
#include "sds_rec.h"

#include "sensor_drv.h"
#include "sensor_config.h"

// Configuration
#ifndef REC_BUF_SIZE_MICROPHONE
#define REC_BUF_SIZE_MICROPHONE             131072
#endif
#ifndef REC_IO_THRESHOLD_MICROPHONE
#define REC_IO_THRESHOLD_MICROPHONE         0
#endif

#ifndef SENSOR_POLLING_INTERVAL
#define SENSOR_POLLING_INTERVAL             150U  /* 150ms */
#endif


// Sensor identifiers
static sensorId_t sensorId_microphone           = NULL;

// Sensor configuration
static sensorConfig_t *sensorConfig_microphone  = NULL;

// Recorder identifiers
static sdsRecId_t recId_microphone              = NULL;

// Recorder buffers
static uint8_t recBuf_microphone[REC_BUF_SIZE_MICROPHONE];

// Sensor close flag
static uint8_t close_flag = 0U;

// Event close sent flag
static uint8_t event_close_sent;

// Thread identifiers
static osThreadId_t thrId_demo           = NULL;
static osThreadId_t thrId_read_sensors   = NULL;

#define EVENT_BUTTON                    (1U << 0)
#define EVENT_CLOSE                     (1U << 0)

// Read sensor thread
static __NO_RETURN void read_sensors (void *argument) {
  void *block;
  uint32_t num, block_size;
  uint32_t timestamp;
  (void)   argument;

  block_size = sensorConfig_microphone->u.dma.block_size;
  timestamp = osKernelGetTickCount();
  for (;;) {
    if (close_flag == 0U) {
      if (sensorGetStatus(sensorId_microphone).active != 0U) {
        block = sensorGetBlockData(sensorId_microphone);
        if (block != NULL) {
          num = sdsRecWrite(recId_microphone, timestamp, block, block_size);
          if (num != block_size) {
            printf("%s: Recorder write failed\r\n", sensorConfig_microphone->name);
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

// Recorder event callback
static void recorder_event_callback (sdsRecId_t id, uint32_t event) {
  if (event & SDS_REC_EVENT_IO_ERROR) {
    if (id == recId_microphone) {
      printf("%s: Recorder event - I/O error\r\n", sensorConfig_microphone->name);
    }
  }
}

// button_event
static void button_event (void) {
         uint32_t flags;
  static uint8_t  active = 0U;

  if (active == 0U) {
    active = 1U;

    // Microphone enable
    // Open Recorder
    recId_microphone = sdsRecOpen("Microphone",
                                  recBuf_microphone,
                                  sizeof(recBuf_microphone),
                                  REC_IO_THRESHOLD_MICROPHONE);
    sensorEnable(sensorId_microphone);
    printf("Microphone enabled\r\n");

  } else {
    event_close_sent = 0U;
    close_flag = 1U;
    flags = osThreadFlagsWait(EVENT_CLOSE, osFlagsWaitAny, osWaitForever);
    if ((flags & osFlagsError) == 0U) {
      active = 0U;

      // Microphone disable
      sensorDisable(sensorId_microphone);
      // Close Recorder
      sdsRecClose(recId_microphone);
      recId_microphone = NULL;
      printf("Microphone disabled\r\n");
    }

    close_flag = 0U;
  }
}

// Sensor Demo
void __NO_RETURN demo(void) {
  uint32_t flags;

  thrId_demo = osThreadGetId();

  // Get sensor identifier
  sensorId_microphone = sensorGetId("Microphone");

  // Get sensor configuration
  sensorConfig_microphone = sensorGetConfig(sensorId_microphone);

  // Initialize recorder
  sdsRecInit(recorder_event_callback);

  // Create sensor thread
  thrId_read_sensors = osThreadNew(read_sensors, NULL, NULL);

  // Create button thread
  osThreadNew(button, NULL, NULL);

  for(;;) {
    flags = osThreadFlagsWait(EVENT_BUTTON, osFlagsWaitAny, osWaitForever);
    if ((flags & osFlagsError) == 0U) {

      // Button pressed event
      if (flags & EVENT_BUTTON) {
        printf("Button pressed\r\n");
        button_event();
      }
    }
  }
}
