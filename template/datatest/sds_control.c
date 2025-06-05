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
 */

#include <stdio.h>
#include <stdbool.h>
#include "cmsis_os2.h"
#include "cmsis_vio.h"
#include "sds_main.h"
#include "sds_control.h"
#include "sds_rec_play.h"


// AlgorithmThread thread attributes
osThreadAttr_t attrAlgorithmThread = {
  .name = "Algorithm"
};

// SDS error information
sdsError_t sdsError = { 0U, 0U, NULL, 0U };

// SDS streams status
volatile uint8_t sdsStreamingState = SDS_STREAMING_INACTIVE;

// Idle time counter
static volatile uint32_t idle_cnt;

// Recorder/player event callback
static void rec_play_event_callback (sdsRecPlayId_t id, uint32_t event) {
  if ((event & SDS_REC_PLAY_EVENT_ERROR_IO) != 0U) {
    SDS_ASSERT(false);
  }
  if ((event & SDS_REC_EVENT_ERROR_NO_SPACE) != 0U) {
    SDS_ASSERT(false);
  }
  if ((event & SDS_PLAY_EVENT_ERROR_NO_DATA) != 0U) {
    SDS_ASSERT(false);
  }

}

// Recording/playback control thread function.
// Toggle recording/playback via USER push-button.
// Toggle LED0 every 1 second to see that the thread is alive.
// Turn on LED1 when recording/playback is started, turn it off when recording/playback is stopped.
__NO_RETURN void sdsControlThread (void *argument) {
  uint8_t btn_val, keypress;
  uint8_t btn_prev = 0U;
  uint8_t led0_val = 0U;
  uint32_t no_load_cnt, prev_cnt;
  uint32_t interval_time, cnt = 0U;

  // Initialize idle counter
  idle_cnt = 0U;
  osDelay(10U);
  no_load_cnt = idle_cnt;

  // Initialize SDS recorder/player
  if (sdsRecPlayInit(rec_play_event_callback) != SDS_REC_PLAY_OK) {
    printf("SDS initialization failed!\n");
    printf("For Network and USB SDSIO Interfaces ensure that SDSIO Server is running and restart the application!\n");
  }

  // Create algorithm thread
  if (osThreadNew(AlgorithmThread, NULL, &attrAlgorithmThread) == NULL) {
    printf("Algorithm Thread creation failed!\n");
    osThreadExit();
  }

  interval_time = osKernelGetTickCount();
  prev_cnt      = idle_cnt;

  for (;;) {
    // Monitor user button
    btn_val  = vioGetSignal(vioBUTTON0);
    keypress = btn_val & ~btn_prev;
    btn_prev = btn_val;

    // Control SDS recorder/player
    switch (sdsStreamingState) {
      case SDS_STREAMING_INACTIVE:
        if (!keypress) break;

        if (OpenStreams() == 0) {
          // Turn LED1 on
          vioSetSignal(vioLED1, vioLEDon);
          sdsStreamingState = SDS_STREAMING_ACTIVE;
        }
        break;

      case SDS_STREAMING_ACTIVE:
        if (!keypress) break;

        // Request to stop streaming
        sdsStreamingState = SDS_STREAMING_STOP;
        break;

      case SDS_STREAMING_STOP_SAFE:
        if (CloseStreams() == 0) {
          // Turn LED1 off
          vioSetSignal(vioLED1, vioLEDoff);
          sdsStreamingState = SDS_STREAMING_INACTIVE;
        }
        break;
    }

    interval_time += 100U;
    osDelayUntil(interval_time);

    // Do 1 second interval
    if (++cnt == 10U) {
      cnt = 0U;

      // Print idle factor
      printf("%d%% idle\n",(idle_cnt - prev_cnt) / no_load_cnt);
      prev_cnt = idle_cnt;

      // Toggle LED0
      led0_val ^= 1U;
      vioSetSignal(vioLED0, led0_val);
    }
  }
}

// Measure system idle time
__NO_RETURN void osRtxIdleThread(void *argument) {
  (void)argument;

  for (;;) {
    idle_cnt++;
  }
}
