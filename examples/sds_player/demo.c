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
 */

#include <stdio.h>
#include <math.h>

#include "main.h"
#include "cmsis_os2.h"

#include "sds_play.h"

// Configuration
#ifndef PLAY_BUF_SIZE_ACCELEROMETER
#define PLAY_BUF_SIZE_ACCELEROMETER     8192U
#endif

#ifndef PLAY_IO_THRESHOLD_ACCELEROMETER
#define PLAY_IO_THRESHOLD_ACCELEROMETER 7000U
#endif

#ifndef TEMP_BUF_SIZE
#define TEMP_BUF_SIZE                   8192U
#endif

#define ACCELEROMETER_SAMPLE_SIZE       6

#define ACCELEROMETER_FULL_SCALE        2     // Accelerometer full scale configuration: +-2G
#define MOTION_DETECTION_SENSITIVITY_mG 50

#define ACCELEROMETER_ABSOLUTE_LIMIT    (((double)INT16_MAX / (double)ACCELEROMETER_FULL_SCALE) * (1.0 + ((double)MOTION_DETECTION_SENSITIVITY_mG / 1000.0)))

// Player identifier
static sdsPlayId_t playId_accelerometer;

// Player buffer
static uint8_t playBuf_accelerometer[PLAY_BUF_SIZE_ACCELEROMETER];

// Temporary buffer
static uint8_t tempBuf[TEMP_BUF_SIZE];

// Player event callback
static void player_event_callback (sdsPlayId_t id, uint32_t event) {
  if (event & SDS_PLAY_EVENT_IO_ERROR) {
    if (id == playId_accelerometer) {
      printf("Player event - I/O error\r\n");
    }
  }
}

// Sensor Demo
static __NO_RETURN void demo (void *argument) {
  uint32_t  timestamp, tick;
  uint32_t  n, size;
  uint16_t *pbuf;
  double    abs;
  double    abs_max;
  (void)argument;

  // Initialize player
  sdsPlayInit(player_event_callback);

  for (;;)  {
    // Open SDS Player
    playId_accelerometer = sdsPlayOpen("Accelerometer",
                                        playBuf_accelerometer,
                                        sizeof(playBuf_accelerometer),
                                        PLAY_IO_THRESHOLD_ACCELEROMETER);

    tick = osKernelGetTickCount();

    if (playId_accelerometer == NULL) {
      printf("End of example\r\n");
      while(1);
    }

    for(;;) {
      do {
        size = sdsPlayGetSize(playId_accelerometer);
        if (size > sizeof(tempBuf)) {
          // Fatal error: Should not happen
          printf("Error: Record size is bigger than buffer size\r\n");
          while(1);
        }
        if (size == 0U) {
          if (sdsPlayEndOfStream(playId_accelerometer) != 0) {
            sdsPlayClose(playId_accelerometer);
            break;
          }
        }
      } while (size == 0U);

      if (size == 0U) {
        // End of stream
        break;
      }

      sdsPlayRead(playId_accelerometer, &timestamp, tempBuf, size);

      osDelayUntil(tick + timestamp);

      abs_max = 0.0;
      pbuf = (uint16_t *)tempBuf;
      for (n = size / ACCELEROMETER_SAMPLE_SIZE; n != 0U; n--) {
        abs = sqrt((pbuf[0] * pbuf[0]) + (pbuf[1] * pbuf[1]) + (pbuf[2] * pbuf[2]));
        if (abs > abs_max) {
          // Save max value
          abs_max = abs;
        }
        pbuf += 3;
      }
      if (abs_max > ACCELEROMETER_ABSOLUTE_LIMIT) {
        printf("Motion detected!\r\n");
      } else {
        printf("Motion not detected!\r\n");
      }
    }
  }
}

// Application initialization
int32_t app_initialize (void) {
  osThreadNew(demo, NULL, NULL);
  return 0;
}
