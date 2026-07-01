/*
 * Copyright (c) 2025-2026 Arm Limited. All rights reserved.
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

#include <stdbool.h>
#include <stdio.h>
#include "RTE_Components.h"
#include CMSIS_device_header
#include "cmsis_os2.h"
#include "cmsis_vio.h"
#include "sds.h"
#include "sds_main.h"
#include "sds_control.h"


// AlgorithmThread thread attributes
osThreadAttr_t attrAlgorithmThread = {
  .name = "Algorithm"
};

// sdsControlThread thread attributes
osThreadAttr_t attr_sdsControlThread = {
  .name = "sdsControl"
};

// Idle time counter
static volatile uint32_t idle_cnt     = 0U;
static volatile uint8_t  rst_idle_cnt = 0U;
static          uint32_t no_load_cnt  = 0U;

#ifdef RTE_CMSIS_RTOS2_RTX5
// Measure system idle time if OS is RTX5
__NO_RETURN void osRtxIdleThread (void *argument) {
  (void)argument;

  for (;;) {
    idle_cnt++;

    if (rst_idle_cnt != 0U) {           // If request to reset idle_cnt is set
      rst_idle_cnt = 0U;
      idle_cnt = 0U;
    }
  }
}
#else
#warning Idle rate measurement not implemented.
#endif

// Calibrate idle measurement
void sdsIdleCalibrate (void) {
  osDelay(1U);
  rst_idle_cnt = 1U;
  osDelay(10U);
  no_load_cnt = idle_cnt;
}

// Update sdsIdleRate and print it every 1 second (should be called every 100 ms).
void sdsIdleUpdate (void) {
  static uint8_t  cnt = 0U;
  static uint32_t idle_cnt_prev = 0U;
  static uint32_t tick_cnt_prev = 0U;
  uint32_t tick_cnt;
  uint32_t idle_rate;

  // Do 1 second interval
  if (++cnt == 10U) {
    cnt = 0U;
    tick_cnt = osKernelGetTickCount();

    // Calculate idle rate and store it into sdsIdleRate
    if ((no_load_cnt != 0U) && (idle_cnt_prev != 0U)) {
      idle_rate = (idle_cnt - idle_cnt_prev) / no_load_cnt;
      if ((tick_cnt - tick_cnt_prev) != 1000U) {
        // If operations such as opening a stream takes longer than 100 ms,
        // the effective measurement interval may exceed 1 second.
        // In that case, we recalculate the idle time accordingly.
        idle_rate = (idle_rate * 1000U) / (tick_cnt - tick_cnt_prev);
      }
      if (idle_rate > 100U) {
        // Clamp to 100 %
        idle_rate = 100U;
      }
      sdsIdleRate = idle_rate;

      // Print idle rate
      SDS_PRINTF("%d%% idle\n", sdsIdleRate);
    } else {
      // Idle rate not valid
      sdsIdleRate = 0xFFFFFFFFU;
    }
    tick_cnt_prev = tick_cnt;
    idle_cnt_prev = idle_cnt;
  }
}

// SDS status LED (should be called every 100 ms).
void sdsStatusLED (void) {
  static uint8_t ticks = 0U;
  static uint8_t led_state = 0U;

  // User code for a status LED:
  // - When disconnected        100ms on,    2s off
  // - During streaming off:       1s on,    1s off
  // - During streaming active: 100ms on, 100ms off

  if (ticks == 0U) {
    if (led_state == 0U) {              // If LED is currently off
      vioSetSignal(vioLED0, vioLEDon);
      switch (sdsState) {
        case SDS_STATE_INACTIVE:        // Disconnected
        case SDS_STATE_ACTIVE:          // Streaming
          ticks = 1U;                   // on for 100ms
          break;
        default:                        // Connected but not streaming
          ticks = 10U;                  // on for 1s
          break;
      }
    } else {                            // If LED is currently on
      vioSetSignal(vioLED0, vioLEDoff);
      switch (sdsState) {
        case SDS_STATE_INACTIVE:        // Disconnected
          ticks = 20U;                  // off for 2s
          break;
        case SDS_STATE_ACTIVE:          // Streaming
          ticks = 1U;                   // off for 100ms
          break;
        default:                        // Connected but not streaming
          ticks = 10U;                  // off for 1s
          break;
      }
    }    
    led_state ^= 1U;
  }  
  ticks--;
}

// SDS event callback
static void sds_event_callback (sdsId_t id, uint32_t event) {
  (void)id;

  if ((event & SDS_EVENT_ERROR_IO) != 0U) {
    SDS_ASSERT(false);
  }
  if ((event & SDS_EVENT_NO_SPACE) != 0U) {
    // Add code for handling no space event if necessary
  }
  if ((event & SDS_EVENT_NO_DATA) != 0U) {
    // Add code for handling no data event if necessary
  }
}


// SDS control thread function.
__NO_RETURN void sdsControlThread (void *argument) {
  uint8_t  btn_val, keypress, btn_prev = 0U;
  uint32_t interval_time;
  int32_t  ret;

  (void)argument;

  // Calibrate idle counter on 10 ms interval
  sdsIdleCalibrate();

  // Initialize SDS system
  ret = sdsInit(sds_event_callback);
  SDS_ERROR_CHECK(ret);

  // Create algorithm thread
  if (osThreadNew(AlgorithmThread, NULL, &attrAlgorithmThread) == NULL) {
    SDS_PRINTF("Algorithm Thread creation failed!\n");
    osThreadExit();
  }

  interval_time = osKernelGetTickCount();

  for (;;) {
    sdsExchange();                              // Exchange control information with host

    // Detect if user button was pressed
    btn_val  = vioGetSignal(vioBUTTON0);
    keypress = btn_val & ~btn_prev;
    btn_prev = btn_val;

    if (keypress) {                             // If button was pressed
      if ((sdsFlags & SDS_FLAG_START) == 0U) {  // If streaming is not active, start it
        sdsFlagsModify(SDS_FLAG_START, 0U);
      } else {                                  // If streaming is active, stop it
        sdsFlagsModify(0U, SDS_FLAG_START);
      }
    }

    // Handle switch to states TERMINATE, RESET or INACTIVE
    if ((sdsFlags & SDS_FLAG_TERMINATE) != 0U) {
      sdsState = SDS_STATE_TERMINATE;
    } else if ((sdsFlags & SDS_FLAG_RESET) != 0U) {
      sdsState = SDS_STATE_RESET;
    } else if ((sdsFlags & SDS_FLAG_ALIVE) == 0U) {
      sdsState = SDS_STATE_INACTIVE;
    }

    // SDS state machine
    switch (sdsState) {
      case SDS_STATE_INACTIVE:          // Streaming is not active and Device is not connected to Host
        if ((sdsFlags & SDS_FLAG_ALIVE) != 0U) {
          sdsState = SDS_STATE_CONNECTED;
        }
        break;

      case SDS_STATE_CONNECTED:         // Device is connected to Host
        if ((sdsFlags & SDS_FLAG_START) != 0U) {
          sdsState = SDS_STATE_START;
        }
        break;

      case SDS_STATE_START:             // Request to start streaming, open streams and get ready for read/write operations
        if (OpenStreams() == 0) {
          sdsState = SDS_STATE_ACTIVE;
        } else {
          sdsState = SDS_STATE_STOP_DONE;
        }
        // Refresh interval timing as stream open can take more then 100 ms
        interval_time = osKernelGetTickCount();
        break;

      case SDS_STATE_ACTIVE:            // Streaming is active
        // Transition to stop request state will happen from AlgorithmThread
        // when it detects that start flags was cleared in active state or when there is
        // no more playback data
        break;

      case SDS_STATE_STOP_REQ:          // Request to stop streaming and close streams
        (void)CloseStreams();
        sdsState = SDS_STATE_STOP_DONE;
        break;

      case SDS_STATE_STOP_DONE:         // Streaming stopped
        // Clear START flag in sdsFlags
        sdsFlagsModify(0U, SDS_FLAG_START);
        sdsState = SDS_STATE_INACTIVE;
        break;

      case SDS_STATE_END:               // Request to end streaming (no more data)
        sdsState = SDS_STATE_STOP_DONE;
        break;

      case SDS_STATE_RESET:             // Request to reset application
        __NVIC_SystemReset();
        for (;;) {}                     // Wait for reset

      case SDS_STATE_TERMINATE:         // Request to terminate CI run (on FVP simulation or pyOCD)
      default:                          // or unexpected state
        // Clear TERMINATE flag in sdsFlags
        sdsFlagsModify(0U, SDS_FLAG_TERMINATE);
        sdsState = SDS_STATE_INACTIVE;

        // Send signal to terminate simulator or pyOCD
        putchar(0x04);
        break;
    }

    // Track idle time and print idle rate every 1 second
    interval_time += 100U;
    osDelayUntil(interval_time);        // 100 ms

    sdsIdleUpdate();                    // Update sdsIdleRate global variable
    sdsStatusLED();                     // Update status LED according to state
  }
}


// Application main function: application entry point.
int32_t app_main (void) {
  osKernelInitialize();
  osThreadNew(sdsControlThread, NULL, &attr_sdsControlThread);
  osKernelStart();
  return 0;
}
