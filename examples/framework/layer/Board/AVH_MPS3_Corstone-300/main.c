/*---------------------------------------------------------------------------
 * Copyright (c) 2021 Arm Limited (or its affiliates). All rights reserved.
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
 *---------------------------------------------------------------------------*/

#include "RTE_Components.h"
#include  CMSIS_device_header
#include "cmsis_os2.h"
#include "cmsis_vio.h"
#ifdef    CMSIS_shield_header
#include  CMSIS_shield_header
#endif
#ifdef RTE_Compiler_EventRecorder
#include "EventRecorder.h"
#endif

#include "main.h"

#ifdef CMSIS_shield_header
__WEAK int32_t shield_setup (void) {
  return 0;
}
#endif

__WEAK void app_main (void *argument) {
  (void) argument;
}

__WEAK int32_t app_initialize (void) {
  osThreadNew(app_main, NULL, NULL);
  return 0;
}

int main (void) {

  vioInit();                            // Initialize Virtual I/O

#ifndef ARM_VSI_DISABLE
  VSI2_Initialize();                    // Initialize VSI2
  VSI5_Initialize();                    // Initialize VSI5
  VSI6_Initialize();                    // Initialize VSI6
#endif

#ifdef CMSIS_shield_header
  shield_setup();
#endif

#if defined(RTE_Compiler_EventRecorder) && \
    (defined(__MICROLIB) || \
    !(defined(RTE_CMSIS_RTOS2_RTX5) || defined(RTE_CMSIS_RTOS2_FreeRTOS)))
  EventRecorderInitialize(EventRecordAll, 1U);
#endif

  osKernelInitialize();                 // Initialize CMSIS-RTOS2
  app_initialize();                     // Initialize application
  osKernelStart();                      // Start thread execution

  for (;;) {}
}
