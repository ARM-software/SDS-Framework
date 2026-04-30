/*
 * Copyright (c) 2023-2026 Arm Limited. All rights reserved.
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
#include "sds.h"
#include "sdsio.h"
#include "arm_vsi.h"

// SDSIO Peripheral definitions
#define SDSIO           ARM_VSI3

#define COMMAND         Regs[0]         // IO Command
#define STREAM_ID       Regs[1]         // Stream handle
#define ARGUMENT        Regs[2]         // IO parameter

// SDSIO Commands
#define CMD_OPEN        1U
#define CMD_CLOSE       2U
#define CMD_WRITE       3U
#define CMD_READ        4U
#define CMD_FLAGS       6U
#define CMD_INFO        7U

static osSemaphoreId_t lock_id = NULL;

// SDS I/O functions

/**
  \fn          int32_t sdsioInit (void)
  \brief       Initialize SDS I/O interface.
  \return      SDS_OK on success or
               a negative value on error (see \ref SDS_Return_Codes)
*/
int32_t sdsioInit (void) {

  /* Initialize SDSIO peripheral */
  SDSIO->Timer.Control = 0U;
  SDSIO->DMA.Control   = 0U;

  lock_id = osSemaphoreNew (1, 1, NULL);
  if (lock_id == NULL) {
    SDS_PRINTF("SDS I/O VSI interface initialization failed!\n");
    return SDS_ERROR_IO;
  }

  SDS_PRINTF("SDS I/O VSI interface initialized successfully\n");
  return SDS_OK;
}

/**
  \fn          int32_t sdsioUninit (void)
  \brief       Un-initialize SDS I/O interface.
  \return      SDS_OK on success or
               a negative value on error (see \ref SDS_Return_Codes)
*/
int32_t sdsioUninit (void) {

  SDSIO->Timer.Control = 0U;
  SDSIO->DMA.Control   = 0U;

  osSemaphoreDelete (lock_id);
  lock_id = NULL;

  return SDS_OK;
}

/**
  \fn          sdsioId_t sdsioOpen (const char *name, sdsioMode_t mode)
  \brief       Open I/O stream.
  \param[in]   name           stream name (pointer to NULL terminated string)
  \param[in]   mode           \ref sdsioMode_t open mode
  \return      \ref sdsioId_t Handle to SDS I/O stream, or NULL if operation failed
*/
sdsioId_t sdsioOpen (const char *name, sdsioMode_t mode) {
  sdsioId_t id;

  if (name == NULL) {
    return NULL;
  }

  if (osSemaphoreAcquire (lock_id, osWaitForever) != osOK) {
    return NULL;
  }

  /* Copy filename to SDSIO peripheral */
  SDSIO->DMA.Address    = (uint32_t)name;
  SDSIO->DMA.BlockSize  = strlen(name) + 1U;
  SDSIO->DMA.BlockNum   = 1;
  SDSIO->DMA.Control    = ARM_VSI_DMA_Direction_M2P  | ARM_VSI_DMA_Enable_Msk;
  SDSIO->Timer.Interval = 0U;
  SDSIO->Timer.Control  = ARM_VSI_Timer_Trig_DMA_Msk | ARM_VSI_Timer_Run_Msk;

  /* Wait for DMA to complete, then stop it */
  while (SDSIO->Timer.Control & ARM_VSI_Timer_Run_Msk);
  SDSIO->DMA.Control = 0U;

  SDSIO->ARGUMENT    = mode;
  SDSIO->COMMAND     = CMD_OPEN;

  id = (sdsioId_t)SDSIO->STREAM_ID;

  osSemaphoreRelease (lock_id);

  return id;
}

/**
  \fn          int32_t sdsioClose (sdsioId_t id)
  \brief       Close I/O stream.
  \param[in]   id             \ref sdsioId_t handle to SDS I/O stream
  \return      SDS_OK on success or
               a negative value on error (see \ref SDS_Return_Codes)
*/
int32_t sdsioClose (sdsioId_t id) {

  if (id == NULL) {
    return SDS_ERROR_PARAMETER;
  }

  if (osSemaphoreAcquire (lock_id, osWaitForever) != osOK) {
    return SDS_ERROR_IO;
  }

  SDSIO->STREAM_ID = (uint32_t)id;
  SDSIO->COMMAND   = CMD_CLOSE;

  osSemaphoreRelease (lock_id);

  return SDS_OK;
}

/**
  \fn          int32_t sdsioWrite (sdsioId_t id, const void *buf, uint32_t buf_size)
  \brief       Write data to I/O stream.
  \param[in]   id             \ref sdsioId_t handle to SDS I/O stream
  \param[in]   buf            pointer to buffer with data to write
  \param[in]   buf_size       buffer size in bytes
  \return      number of bytes successfully written or
               a negative value on error (see \ref SDS_Return_Codes)
*/
int32_t sdsioWrite (sdsioId_t id, const void *buf, uint32_t buf_size) {
  int32_t retv;

  if ((id == NULL) || (buf == NULL) || (buf_size == 0U)) {
    return SDS_ERROR_PARAMETER;
  }

  if (osSemaphoreAcquire (lock_id, osWaitForever) != osOK) {
    return SDS_ERROR_IO;
  }

  /* Copy data to an SDSIO peripheral */
  SDSIO->DMA.Address    = (uint32_t)buf;
  SDSIO->DMA.BlockSize  = buf_size;
  SDSIO->DMA.BlockNum   = 1;
  SDSIO->DMA.Control    = ARM_VSI_DMA_Direction_M2P  | ARM_VSI_DMA_Enable_Msk;
  SDSIO->Timer.Interval = 0U;
  SDSIO->Timer.Control  = ARM_VSI_Timer_Trig_DMA_Msk | ARM_VSI_Timer_Run_Msk;

  SDSIO->STREAM_ID   = (uint32_t)id;

  /* Wait for DMA to complete, then stop it */
  while (SDSIO->Timer.Control & ARM_VSI_Timer_Run_Msk);
  SDSIO->DMA.Control = 0U;

  /* Write data transferred via DMA to a file */
  SDSIO->COMMAND     = CMD_WRITE;

  /* Return number of bytes written or an error status */
  retv = (int32_t)SDSIO->ARGUMENT;

  osSemaphoreRelease (lock_id);

  return retv;
}

/**
  \fn          int32_t sdsioRead (sdsioId_t id, void *buf, uint32_t buf_size)
  \brief       Read data from I/O stream.
  \param[in]   id             \ref sdsioId_t handle to SDS I/O stream
  \param[out]  buf            pointer to buffer for data to read
  \param[in]   buf_size       buffer size in bytes
  \return      number of bytes successfully read, or
               a negative value on error or SDS_EOS (see \ref SDS_Return_Codes)
*/
int32_t sdsioRead (sdsioId_t id, void *buf, uint32_t buf_size) {
  int32_t retv;

  if ((id == NULL) || (buf == NULL) || (buf_size == 0U)) {
    return SDS_ERROR_PARAMETER;
  }

  if (osSemaphoreAcquire (lock_id, osWaitForever) != osOK) {
    return SDS_ERROR_IO;
  }

  /* Read data from a file to an SDSIO peripheral */
  SDSIO->STREAM_ID = (uint32_t)id;
  SDSIO->ARGUMENT  = buf_size;
  SDSIO->COMMAND   = CMD_READ;

  /* Check return code from fread */
  retv = (int32_t)SDSIO->ARGUMENT;
  if (retv > 0) {
    /* Copy data from an SDSIO peripheral to an application */
    SDSIO->DMA.Address    = (uint32_t)buf;
    SDSIO->DMA.BlockSize  = (uint32_t)retv;
    SDSIO->DMA.BlockNum   = 1;
    SDSIO->DMA.Control    = ARM_VSI_DMA_Direction_P2M  | ARM_VSI_DMA_Enable_Msk;
    SDSIO->Timer.Interval = 0U;
    SDSIO->Timer.Control  = ARM_VSI_Timer_Trig_DMA_Msk | ARM_VSI_Timer_Run_Msk;

    /* Wait for DMA to complete, then stop it */
    while (SDSIO->Timer.Control & ARM_VSI_Timer_Run_Msk);
    SDSIO->DMA.Control = 0U;
  }

  osSemaphoreRelease (lock_id);

  /* Return data len or EOS status */
  return retv;
}

/**
  Ask host for SDSIO_CMD_FLAGS information and update sdsFlags accordingly.
  Send the current sdsFlags value, along with sdsIdleRate and any optional
  error information (sdsError), to the host.
*/
int32_t sdsExchange (void) {
  // Not implemented yet
  return SDS_ERROR_IO;
}
