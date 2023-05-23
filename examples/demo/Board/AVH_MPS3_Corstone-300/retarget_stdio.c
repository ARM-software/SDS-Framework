/*---------------------------------------------------------------------------
 * Copyright (c) 2022 Arm Limited (or its affiliates). All rights reserved.
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
 *      Purpose: Retarget stdio to UART
 *
 *---------------------------------------------------------------------------*/

#include "RTE_Components.h"
#include CMSIS_device_header

#include "device_definition.h"
#include "uart_cmsdk_drv.h"
#include "cmsis_os2.h" 

#define UART_DEV                UART0_CMSDK_DEV
#define UART_BAUDRATE           115200
#define UART_BUF_SIZE           2048    /* must be 2^n */

static uint8_t rx_buf[UART_BUF_SIZE];
volatile static uint32_t rx_idx_i;
volatile static uint32_t rx_idx_o;

#define UART_RX_EVENT 1U
static osEventFlagsId_t event_id = NULL;

/* UART RX IRQ handler */
void UARTRX0_Handler (void);
void UARTRX0_Handler (void) {
  uint32_t idx = rx_idx_i & (UART_BUF_SIZE - 1);

  if (uart_cmsdk_read(&UART_DEV, &rx_buf[idx]) == UART_CMSDK_ERR_NONE) {
    rx_idx_i++;
    osEventFlagsSet(event_id, UART_RX_EVENT);
  }
  uart_cmsdk_clear_interrupt(&UART_DEV, UART_CMSDK_IRQ_RX);
}

/**
  Initialize stdio
 
  \return          0 on success, or -1 on error.
*/
int stdio_init (void) {

  rx_idx_i = 0U;
  rx_idx_o = 0U;

  if (uart_cmsdk_init(&UART_DEV, PeripheralClock) != UART_CMSDK_ERR_NONE) {
    return (-1);
  }
  if (uart_cmsdk_set_baudrate(&UART_DEV, UART_BAUDRATE) != UART_CMSDK_ERR_NONE) {
    return (-1);
  }
  if (uart_cmsdk_irq_rx_enable(&UART_DEV) != UART_CMSDK_ERR_NONE) {
    return (-1);
  }
  NVIC_EnableIRQ(UARTRX0_IRQn);

  return (0);
}

/**
  Put a character to the stderr
 
  \param[in]   ch  Character to output
  \return          The character written, or -1 on write error.
*/
int stderr_putchar (int ch) {

  while (!uart_cmsdk_tx_ready(&UART_DEV));
  uart_cmsdk_write(&UART_DEV, (uint8_t)ch);

  return (ch);
}

/**
  Put a character to the stdout

  \param[in]   ch  Character to output
  \return          The character written, or -1 on write error.
*/
int stdout_putchar (int ch) {

  while (!uart_cmsdk_tx_ready(&UART_DEV));
  uart_cmsdk_write(&UART_DEV, (uint8_t)ch);

  return (ch);
}

/**
  Get a character from the stdio
 
  \return     The next character from the input, or -1 on read error.
*/
int stdin_getchar (void) {
  uint32_t cnt, idx;
  int ch;

  if (event_id == NULL) {
    event_id = osEventFlagsNew(NULL);
  }

  do {
    cnt = rx_idx_i - rx_idx_o;
    if (cnt > 0U) {
      idx = rx_idx_o & (UART_BUF_SIZE - 1U);
      ch = rx_buf[rx_idx_o++];
    }
    else {
      osEventFlagsWait(event_id, UART_RX_EVENT, osFlagsWaitAny, osWaitForever);
    }
  } while (cnt == 0U);

  return ch;
}
