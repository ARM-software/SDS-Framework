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
#include <string.h>
#include "RTE_Components.h"
#include "cmsis_os2.h"
#include "cmsis_vio.h"
#include "sds_main.h"
#include "sds_control.h"
#include "sds_rec_play.h"
#ifdef   RTE_SDS_IO_SOCKET
#include "sdsio_config_socket.h"
#endif


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

#ifdef SIMULATOR
static uint32_t key_cnt = 0U;

// Simulate keypress
static uint32_t simGetSignal (uint32_t mask) {
  uint32_t ret = 0U;

  switch (key_cnt) {
#ifdef SDS_PLAY
    case 20U:                           // At 2 seconds
      ret = mask;                       // Simulate keypress
      break;

    case 1000U:                         // At 100 seconds
      putchar(0x04);                    // Send signal to simulator to shutdown
      break;
#else
    case 10U:                           // At 1 second
      ret = mask;                       // Simulate keypress
      break;

    case 110U:                          // At 11 seconds
      ret = mask;                       // Simulate keypress
      break;

    case 120U:                          // At 12 seconds
      putchar(0x04);                    // Send signal to simulator to shutdown
      break;
#endif
  }
  key_cnt++;

  return ret;
}
#endif

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
  int32_t ret;

  // Initialize idle counter
  idle_cnt = 0U;
  osDelay(10U);
  no_load_cnt = idle_cnt;

  // Initialize SDS recorder/player
  ret = sdsRecPlayInit(rec_play_event_callback);

  // Output a diagnostic message about initialization to the STDOUT channel
  switch (ret) {
    case SDS_REC_PLAY_OK:
#if   defined(RTE_SDS_IO_VSI)
      printf("SDS I/O VSI interface initialized successfully\n");
#elif defined(RTE_SDS_IO_SOCKET)
      printf("Connection to SDSIO-Server at %s:%d established\n", SDSIO_SOCKET_SERVER_IP, SDSIO_SOCKET_SERVER_PORT);
#elif defined(RTE_SDS_IO_USB)
      printf("Connection to SDSIO-Server established via USB interface\n");
#elif defined(RTE_SDS_IO_SERIAL_CMSIS_USART)
      printf("Connection to SDSIO-Server established via USART interface\n");
#elif defined(RTE_SDS_IO_CUSTOM)
      printf("Connection to SDSIO-Server established via custom interface\n");
#elif defined(RTE_SDS_IO_FILE_SYSTEM_MDK_FS)
      printf("SDS I/O File System (MDK-FS) interface initialized successfully\n");
#elif defined(RTE_SDS_IO_FILE_SYSTEM_SEMIHOSTING)
      printf("SDS I/O File System (SemiHosting) interface initialized successfully\n");
#endif
      break;

    case SDS_REC_PLAY_ERROR_IO:
#if   defined(RTE_SDS_IO_VSI)
      printf("SDS I/O VSI interface initialization failed!\n");
#elif defined(RTE_SDS_IO_SOCKET)
      if (strcmp(SDSIO_SOCKET_SERVER_IP, "0.0.0.0") == 0) {
        printf("SDSIO_SOCKET_SERVER_IP address not configured (see sdsio_config_socket.h)!\n");
      } else {
        printf("SDS I/O Network interface initialization failed or 'sdsio-server socket' unavailable at %s:%d !\n", SDSIO_SOCKET_SERVER_IP, SDSIO_SOCKET_SERVER_PORT);
        printf("Ensure that SDSIO-Server is running, then restart the application!\n");
      }
#elif defined(RTE_SDS_IO_USB)
      printf("SDS I/O USB interface initialization failed or 'sdsio-server usb' unavailable!\n");
      printf("Ensure that SDSIO-Server is running, then restart the application!\n");
#elif defined(RTE_SDS_IO_SERIAL_CMSIS_USART)
      printf("SDS I/O USART interface initialization failed or 'sdsio-server serial' unavailable!\n");
      printf("Ensure that SDSIO-Server is running, then restart the application!\n");
#elif defined(RTE_SDS_IO_CUSTOM)
      printf("SDS I/O Custom interface initialization failed!\n");
#elif defined(RTE_SDS_IO_FILE_SYSTEM_MDK_FS)
      printf("SDS I/O File System MDK-FS interface initialization failed!\n");
#elif defined(RTE_SDS_IO_FILE_SYSTEM_SEMIHOSTING)
      printf("SDS I/O File System SemiHosting interface initialization failed!\n");
#endif
      break;

    case SDS_REC_PLAY_ERROR:
      printf("SDS initialization failed to create necessary threads or event flags!\n");
      break;

    default:
      printf("SDS initialization failed with error code: %d\n", ret);
      break;
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
#ifdef SIMULATOR
    btn_val  = simGetSignal(vioBUTTON0);
#else
    btn_val  = vioGetSignal(vioBUTTON0);
#endif
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
#ifdef SDS_PLAY
#ifdef SIMULATOR
        // Start next SDS stream
        key_cnt = 0U;
#endif
#endif
        break;

      case SDS_STREAMING_END:
#ifdef SIMULATOR
        // Send signal to simulator to shutdown
        putchar(0x04);
#endif
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
