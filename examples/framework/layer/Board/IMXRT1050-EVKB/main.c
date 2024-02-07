/*---------------------------------------------------------------------------
 * Copyright (c) 2021-2022 Arm Limited (or its affiliates).
 * All rights reserved.
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
#ifdef RTE_VIO_BOARD
#include "cmsis_vio.h"
#endif
#ifdef    CMSIS_shield_header
#include  CMSIS_shield_header
#endif
#ifdef RTE_Compiler_EventRecorder
#include "EventRecorder.h"
#endif

#include "clock_config.h"
#include "board.h"
#include "pin_mux.h"
#include "peripherals.h"
#include "fsl_iomuxc.h"
#include "fsl_dmamux.h"
#include "fsl_sai_edma.h"
#include "fsl_fxos.h"
#include "main.h"

// Callbacks for LPUART1 Driver
uint32_t LPUART1_GetFreq   (void) { return BOARD_BOOTCLOCKRUN_UART_CLK_ROOT; }
void     LPUART1_InitPins  (void) { /* Done in BOARD_InitDEBUG_UART function */ }
void     LPUART1_DeinitPins(void) { /* Not implemented */ }

// Callbacks for LPUART3 Driver
uint32_t LPUART3_GetFreq   (void) { return BOARD_BOOTCLOCKRUN_UART_CLK_ROOT; }
void     LPUART3_InitPins  (void) { /* Done in BOARD_InitARDUINO_UART function */ }
void     LPUART3_DeinitPins(void) { /* Not implemented */ }

// FXOS 6-axis sensor handle
fxos_handle_t g_fxosHandle = {0};

// Sensor initialize
static void BOARD_InitSensor (void) {
  static fxos_config_t config = {0};

  BOARD_InitI2C();
  BOARD_Accel_I2C_Init();

  config.I2C_SendFunc    = BOARD_Accel_I2C_Send;
  config.I2C_ReceiveFunc = BOARD_Accel_I2C_Receive;
  config.slaveAddress    = 0x1FU;
  if (FXOS_Init(&g_fxosHandle, &config) != kStatus_Success) {
    while (1);
  }
}

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
  edma_config_t DmaConfig;

  BOARD_ConfigMPU();
  BOARD_InitBootPeripherals();
  BOARD_InitBootPins();
  BOARD_InitBootClocks();
  BOARD_InitDebugConsole();

  BOARD_InitSensor();

  NVIC_SetPriority(ENET_IRQn,    8U);
  NVIC_SetPriority(USDHC1_IRQn,  8U);
  NVIC_SetPriority(LPUART3_IRQn, 8U);

  /* Initialize DMAMUX */
  DMAMUX_Init (DMAMUX);

  /* Initialize EDMA */
  EDMA_GetDefaultConfig (&DmaConfig);
  EDMA_Init (DMA0,       &DmaConfig);

  SystemCoreClockUpdate();

  /* Reset Ethernet PHY (Required 100 us delay for PHY power on reset) */
  GPIO_PinWrite(GPIO1,  9U, 0U);
  SDK_DelayAtLeastUs(500U, CLOCK_GetFreq(kCLOCK_CpuClk));
  GPIO_PinWrite(GPIO1,  9U, 1U);

#ifdef RTE_VIO_BOARD
  vioInit();                            // Initialize Virtual I/O
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
