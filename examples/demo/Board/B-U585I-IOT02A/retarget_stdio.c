/*---------------------------------------------------------------------------
 * Copyright (c) 2021-2022 Arm Limited (or its affiliates). All rights reserved.
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
 *
 *      Name:    retarget_stdio.c
 *      Purpose: Retarget stdio to ST-Link (Virtual COM Port)
 *
 *---------------------------------------------------------------------------*/

#include "stm32u5xx_hal.h"
#include "cmsis_os2.h"                  // ::CMSIS:RTOS2


#define HUARTx            huart1
#define UART_BUFFER_SIZE  2048U         // must be 2^n
#define UART_RX_EVENT     1U

extern UART_HandleTypeDef HUARTx;

extern int stderr_putchar (int ch);
extern int stdout_putchar (int ch);
extern int stdin_getchar  (void);

// Local Variables
         static uint8_t  uart_rx_initialized = 0U;
         static uint8_t  uart_rx_buf[UART_BUFFER_SIZE];
volatile static uint32_t uart_rx_idx_i;
volatile static uint32_t uart_rx_idx_o;

static osEventFlagsId_t  uart_rx_evt_id;


/**
  Uart receive callback
*/
static void uart_rx_callback (UART_HandleTypeDef * huart, uint16_t num) {
  uint32_t idx, cnt;

  // UART data received, restart new reception
  uart_rx_idx_i += num;
  idx = uart_rx_idx_i & (UART_BUFFER_SIZE - 1U);
  cnt = UART_BUFFER_SIZE - idx;
  HAL_UARTEx_ReceiveToIdle_IT(&HUARTx, &uart_rx_buf[idx], cnt);

  osEventFlagsSet(uart_rx_evt_id, UART_RX_EVENT);
}

/**
  Uart receive initialize
*/
static void uart_rx_init (void) {

  uart_rx_idx_i = 0U;
  uart_rx_idx_o = 0U;

  uart_rx_evt_id = osEventFlagsNew(NULL);

  HAL_UART_RegisterRxEventCallback(&HUARTx, uart_rx_callback);
  HAL_UARTEx_ReceiveToIdle_IT(&HUARTx, uart_rx_buf, UART_BUFFER_SIZE);
}

/**
  Put a character to the stderr
 
  \param[in]   ch  Character to output
  \return          The character written, or -1 on write error.
*/
int stderr_putchar (int ch) {

  if (HAL_UART_Transmit(&HUARTx, (uint8_t *)&ch, 1U, 1000U) != HAL_OK) {
    return -1;
  }

  return ch;
}

/**
  Put a character to the stdout

  \param[in]   ch  Character to output
  \return          The character written, or -1 on write error.
*/
int stdout_putchar (int ch) {

  if (HAL_UART_Transmit(&HUARTx, (uint8_t *)&ch, 1U, 1000U) != HAL_OK) {
    return -1;
  }

  return ch;
}

/**
  Get a character from the stdio
 
  \return     The next character from the input, or -1 on read error.
*/
int stdin_getchar (void) {
  int ch = -1;
  uint32_t idx, cnt;

  if (uart_rx_initialized == 0U) {
    uart_rx_init();
    uart_rx_initialized = 1U;
  }

  do {
    cnt = uart_rx_idx_i - uart_rx_idx_o;
    if (cnt > 0U) {
      idx = uart_rx_idx_o & (UART_BUFFER_SIZE - 1U);
      ch = uart_rx_buf[idx];
      uart_rx_idx_o++;
    }
    else {
      osEventFlagsWait(uart_rx_evt_id, UART_RX_EVENT, osFlagsWaitAny, osWaitForever);
    }
  } while (cnt == 0U);

  return ch;
}
