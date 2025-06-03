/*
 * Copyright (c) 2023-2025 Arm Limited. All rights reserved.
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

// SDS I/O interface via VSI

#include <stddef.h>

#include <string.h>
#include "cmsis_os2.h"
#include "sdsio.h"
#include "arm_vsi.h"

// SDSIO Peripheral definitions
#define SDSIO           ARM_VSI3

#define STATUS          Regs[0]         // Timer status
#define COMMAND         Regs[1]         // IO Command
#define STREAM_ID       Regs[2]         // Stream handle
#define ARGUMENT        Regs[3]         // IO parameter

// SDSIO Commands
#define CMD_OPEN        1U
#define CMD_CLOSE       2U
#define CMD_WRITE       3U
#define CMD_READ        4U

static osSemaphoreId_t lock_id = NULL;

// SDS I/O functions

/* Initialize I/O interface */
int32_t sdsioInit (void) {

  /* Initialize SDSIO peripheral */
  SDSIO->Timer.Control = 0U;
  SDSIO->DMA.Control   = 0U;

  lock_id = osSemaphoreNew (1, 1, NULL);
  if (lock_id == NULL) {
    return SDSIO_ERROR;
  }

  return SDSIO_OK;
}

/* Un-initialize I/O interface */
int32_t sdsioUninit (void) {

  SDSIO->DMA.Control   = 0U;
  SDSIO->Timer.Control = 0U;

  osSemaphoreDelete (lock_id);
  lock_id = NULL;

  return SDSIO_OK;
}

/* Open I/O stream */
sdsioId_t sdsioOpen (const char *name, sdsioMode_t mode) {
  sdsioId_t id;

  if (name == NULL) {
    return NULL;
  }

  if (osSemaphoreAcquire (lock_id, osWaitForever) != osOK) {
    return NULL;
  }

  /* Copy filename to SDSIO peripheral */
  SDSIO->DMA.Address   = (uint32_t)name;
  SDSIO->DMA.BlockSize = strlen(name) + 1U;
  SDSIO->DMA.BlockNum  = 1;
  SDSIO->DMA.Control   = ARM_VSI_DMA_Direction_M2P  | ARM_VSI_DMA_Enable_Msk;
  SDSIO->Timer.Control = ARM_VSI_Timer_Trig_DMA_Msk | ARM_VSI_Timer_Run_Msk;

  /* Wait for DMA to complete, then stop it */
  while (SDSIO->STATUS & ARM_VSI_Timer_Run_Msk);
  SDSIO->DMA.Control   = 0U;

  SDSIO->ARGUMENT      = mode;
  SDSIO->COMMAND       = CMD_OPEN;

  id = (sdsioId_t)SDSIO->STREAM_ID;

  osSemaphoreRelease (lock_id);

  return id;
}

/* Close I/O stream */
int32_t sdsioClose (sdsioId_t id) {

  if (id == NULL) {
    return SDSIO_ERROR_PARAMETER;
  }

  if (osSemaphoreAcquire (lock_id, osWaitForever) != osOK) {
    return SDSIO_ERROR;
  }

  SDSIO->STREAM_ID = (uint32_t)id;
  SDSIO->COMMAND   = CMD_CLOSE;

  osSemaphoreRelease (lock_id);

  return SDSIO_OK;
}

/* Write data to I/O stream */
int32_t sdsioWrite (sdsioId_t id, const void *buf, uint32_t buf_size) {
  int32_t retv;

  if ((id == NULL) || (buf == NULL) || (buf_size == 0U)) {
    return SDSIO_ERROR_PARAMETER;
  }

  if (osSemaphoreAcquire (lock_id, osWaitForever) != osOK) {
    return SDSIO_ERROR;
  }

  /* Copy data to an SDSIO peripheral */
  SDSIO->DMA.Address   = (uint32_t)buf;
  SDSIO->DMA.BlockSize = buf_size;
  SDSIO->DMA.BlockNum  = 1;
  SDSIO->DMA.Control   = ARM_VSI_DMA_Direction_M2P  | ARM_VSI_DMA_Enable_Msk;
  SDSIO->Timer.Control = ARM_VSI_Timer_Trig_DMA_Msk | ARM_VSI_Timer_Run_Msk;

  SDSIO->STREAM_ID     = (uint32_t)id;

  /* Wait for DMA to complete, then stop it */
  while (SDSIO->STATUS & ARM_VSI_Timer_Run_Msk);
  SDSIO->DMA.Control   = 0U;

  /* Write data transferred via DMA to a file */
  SDSIO->COMMAND       = CMD_WRITE;

  /* Return number of bytes written or an error status */
  retv = (int32_t)SDSIO->ARGUMENT;

  osSemaphoreRelease (lock_id);

  return retv;
}

/* Read data from I/O stream */
int32_t sdsioRead (sdsioId_t id, void *buf, uint32_t buf_size) {
  int32_t retv;

  if ((id == NULL) || (buf == NULL) || (buf_size == 0U)) {
    return SDSIO_ERROR_PARAMETER;
  }

  if (osSemaphoreAcquire (lock_id, osWaitForever) != osOK) {
    return SDSIO_ERROR;
  }

  /* Read data from a file to an SDSIO peripheral */
  SDSIO->STREAM_ID     = (uint32_t)id;
  SDSIO->ARGUMENT      = buf_size;
  SDSIO->COMMAND       = CMD_READ;

  /* Check return code from fread */
  retv = (int32_t)SDSIO->ARGUMENT;
  if (retv > 0) {
    /* Copy data from an SDSIO peripheral to an application */
    SDSIO->DMA.Address   = (uint32_t)buf;
    SDSIO->DMA.BlockSize = (uint32_t)retv;
    SDSIO->DMA.BlockNum  = 1;
    SDSIO->DMA.Control   = ARM_VSI_DMA_Direction_P2M  | ARM_VSI_DMA_Enable_Msk;
    SDSIO->Timer.Control = ARM_VSI_Timer_Trig_DMA_Msk | ARM_VSI_Timer_Run_Msk;

    /* Wait for DMA to complete, then stop it */
    while (SDSIO->STATUS & ARM_VSI_Timer_Run_Msk);
    SDSIO->DMA.Control   = 0U;
  }

  osSemaphoreRelease (lock_id);

  /* Return data len or EOS status */
  return retv;
}
