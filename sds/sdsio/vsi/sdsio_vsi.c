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

// SDSIO via VSI

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
#define FLAGS_SET       Regs[3]         // Flags set mask
#define FLAGS_CLR       Regs[4]         // Flags clear mask

// SDSIO Commands
#define CMD_OPEN        1U
#define CMD_CLOSE       2U
#define CMD_WRITE       3U
#define CMD_READ        4U
#define CMD_FLAGS       6U
#define CMD_INFO        7U

#ifndef SDSIO_VSI_ERROR_MAX_DATA_SIZE
#define SDSIO_VSI_ERROR_MAX_DATA_SIZE  128U
#endif

static osSemaphoreId_t lock_id = NULL;
static uint8_t         error_data[SDSIO_VSI_ERROR_MAX_DATA_SIZE];

// SDSIO functions

/**
  \fn          int32_t sdsioInit (void)
  \brief       Initialize SDSIO interface.
  \return      SDS_OK on success or
               a negative value on error (see \ref SDS_Return_Codes)
*/
int32_t sdsioInit (void) {

  /* Initialize SDSIO peripheral */
  SDSIO->Timer.Control = 0U;
  SDSIO->DMA.Control   = 0U;

  lock_id = osSemaphoreNew (1, 1, NULL);
  if (lock_id == NULL) {
    SDS_PRINTF("SDSIO VSI interface initialization failed!\n");
    return SDS_ERROR_IO;
  }

  SDS_PRINTF("SDSIO VSI interface initialized successfully\n");
  return SDS_OK;
}

/**
  \fn          int32_t sdsioUninit (void)
  \brief       Un-initialize SDSIO interface.
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
  \brief       Open SDSIO stream.
  \param[in]   name           stream name (pointer to NULL terminated string)
  \param[in]   mode           \ref sdsioMode_t open mode
  \return      \ref sdsioId_t Handle to SDSIO stream, or NULL if operation failed
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
  \brief       Close SDSIO stream.
  \param[in]   id             \ref sdsioId_t handle to SDSIO stream
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
  \brief       Write data to SDSIO stream.
  \param[in]   id             \ref sdsioId_t handle to SDSIO stream
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
  \brief       Read data from SDSIO stream.
  \param[in]   id             \ref sdsioId_t handle to SDSIO stream
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
  Check whether asynchronous SDSIO_CMD_FLAGS information has been received
  from the host, and update sdsFlags accordingly.
  Read:
    header: command   = SDSIO_CMD_FLAGS
            sdsio_id  = set mask
            argument  = clear mask
            data_size = 0

  Send the current sdsFlags value, along with sdsIdleRate and any optional
  error information (sdsError), to the host.
  Send:
    header: command   = SDSIO_CMD_INFO
            sdsio_id  = sdsFlags
            argument  = sdsIdleRate
            data_size = number of error data bytes to send
    data:   error data to be sent
*/
int32_t sdsExchange (void) {
  uint32_t set_mask;
  uint32_t clr_mask;
  uint32_t ofs = 0U;
  uint32_t len = 0U;

  if (osSemaphoreAcquire (lock_id, osWaitForever) != osOK) {
    return SDS_ERROR_IO;
  }

  /* Request pending control flag changes from the VSI host. */
  SDSIO->COMMAND = CMD_FLAGS;

  set_mask = SDSIO->FLAGS_SET;
  clr_mask = SDSIO->FLAGS_CLR;
  sdsFlagsModify(set_mask, clr_mask);

  /* Send target status back only while the host reports it is alive. */
  if ((sdsFlags & SDS_FLAG_ALIVE) != 0U) {
    if (sdsError.occurred != 0U) {
      sdsError.occurred = 0U;
      memcpy(error_data,       &sdsError.status, sizeof(sdsError.status));
      ofs  = sizeof(sdsError.status);
      memcpy(error_data + ofs, &sdsError.line,   sizeof(sdsError.line));
      ofs += sizeof(sdsError.line);
      if (sdsError.file != NULL) {
        len = strlen(sdsError.file);
        if (len > (sizeof(error_data) - ofs)) {
          len = sizeof(error_data) - ofs;
        }
        memcpy(error_data + ofs, sdsError.file, len);
        ofs += len;
      }
    }

    if (ofs != 0U) {
      SDSIO->DMA.Address    = (uint32_t)error_data;
      SDSIO->DMA.BlockSize  = ofs;
      SDSIO->DMA.BlockNum   = 1U;
      SDSIO->DMA.Control    = ARM_VSI_DMA_Direction_M2P  | ARM_VSI_DMA_Enable_Msk;
      SDSIO->Timer.Interval = 0U;
      SDSIO->Timer.Control  = ARM_VSI_Timer_Trig_DMA_Msk | ARM_VSI_Timer_Run_Msk;

      while (SDSIO->Timer.Control & ARM_VSI_Timer_Run_Msk);
      SDSIO->DMA.Control = 0U;
    }

    SDSIO->STREAM_ID = sdsFlags;
    SDSIO->ARGUMENT  = sdsIdleRate;
    SDSIO->COMMAND   = CMD_INFO;
  }

  osSemaphoreRelease (lock_id);

  return SDS_OK;
}
