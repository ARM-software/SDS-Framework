/*---------------------------------------------------------------------------
 * Copyright (c) 2021-2023 Arm Limited (or its affiliates).
 * All rights reserved.
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

#include "Audio_B-U585I-IOT02A.h"

static AUDIO_CB Cb = {0};

// Convert BSP error code to Audio Driver return code
static int32_t err_to_audio (int32_t bsp_err) {
  int32_t rc;

  switch (bsp_err) {
    case BSP_ERROR_NONE:
      rc = AUDIO_DRV_OK;
      break;
    case BSP_ERROR_BUSY:
      rc = AUDIO_DRV_ERROR_BUSY;
      break;
    case BSP_ERROR_WRONG_PARAM:
      rc = AUDIO_DRV_ERROR_PARAMETER;
      break;
    case BSP_ERROR_FEATURE_NOT_SUPPORTED:
      rc = AUDIO_DRV_ERROR_UNSUPPORTED;
      break;
    default:
      rc = AUDIO_DRV_ERROR;
      break;
  }
  return (rc);
}

// Free local buffer
static void free_buf (void) {
  if (Cb.dma_buf != NULL) {
    free (Cb.dma_buf);
    Cb.dma_buf = NULL;
  }
}

/**
  \brief       Initialize Audio Interface.
  \param[in]   cb_event Pointer to \ref AudioDrv_Event_t
  \return      return code
*/
int32_t AudioDrv_Initialize (AudioDrv_Event_t cb_event) {
  Cb.callback = cb_event;
  return AUDIO_DRV_OK;
}

/**
  \brief       De-initialize Audio Interface.
  \return      return code
*/
int32_t AudioDrv_Uninitialize (void) {
  int32_t err;

  err = BSP_AUDIO_IN_DeInit (0);
  if (err == BSP_ERROR_NONE) {
    Cb.initialized = 0U;
  }
  return err_to_audio (err);
}

/**
  \brief       Configure Audio Interface.
  \param[in]   interface   Audio Interface
  \param[in]   channels    Number of channels
  \param[in]   sample_bits Sample number of bits (8..32)
  \param[in]   sample_rate Sample rate (samples per second)
  \return      return code
*/
int32_t AudioDrv_Configure (uint32_t interface, uint32_t channels, uint32_t sample_bits, uint32_t sample_rate) {
  BSP_AUDIO_Init_t AudioInit;
  int32_t err;

  if (Cb.initialized != 0U) {
    return AUDIO_DRV_ERROR;
  }

  switch (interface) {
    case AUDIO_DRV_INTERFACE_RX:
      break;
    case AUDIO_DRV_INTERFACE_TX:
      /* Microphone is input only */
      return AUDIO_DRV_ERROR_UNSUPPORTED;
    default:
      return AUDIO_DRV_ERROR_PARAMETER;
  }

  switch (channels) {
    case 1U:
      /* Mono */
      AudioInit.Device = AUDIO_IN_DEVICE_DIGITAL_MIC1;
      break;
    case 2U:
      /* Stereo */
      AudioInit.Device = AUDIO_IN_DEVICE_DIGITAL_MIC;
      break;
    default:
      return AUDIO_DRV_ERROR_PARAMETER;
  }

  if (sample_bits != 16U) {
    return AUDIO_DRV_ERROR_PARAMETER;
  }

  if ((sample_rate < 8000U) || (sample_rate > 192000U)) {
    return AUDIO_DRV_ERROR_PARAMETER;
  }

  AudioInit.BitsPerSample = sample_bits;
  AudioInit.SampleRate    = sample_rate;
  AudioInit.ChannelsNbr   = channels;
  AudioInit.Volume        = 100; /* Not used */

  /* Initialize the driver */
  err = BSP_AUDIO_IN_Init (0, &AudioInit);
  BSP_AUDIO_IN_SetVolume(0, 100);
  if (err == BSP_ERROR_NONE) {
    Cb.initialized = 1U;
  }
  return err_to_audio (err);
}

/**
  \brief       Set Audio Interface buffer.
  \param[in]   interface   Audio Interface
  \param[in]   buf         Pointer to buffer for audio data
  \param[in]   block_num   Number of blocks in buffer (must be 2^n)
  \param[in]   block_size  Block size in number of samples
  \return      return code
*/
int32_t AudioDrv_SetBuf (uint32_t interface, void *buf, uint32_t block_num, uint32_t block_size) {
  uint32_t bsp_state;

  if (Cb.initialized == 0U) {
    return AUDIO_DRV_ERROR;
  }

  if ((buf == NULL) || (block_num < 2U) || (block_size == 0U)) {
    return AUDIO_DRV_ERROR_PARAMETER;
  }

  switch (interface) {
    case AUDIO_DRV_INTERFACE_TX:
      /* Microphone is input only */
      return AUDIO_DRV_ERROR_UNSUPPORTED;
    case AUDIO_DRV_INTERFACE_RX:
      BSP_AUDIO_IN_GetState (0U, &bsp_state);
      if (bsp_state == AUDIO_IN_STATE_RECORDING) {
        return AUDIO_DRV_ERROR_BUSY;
      }
      /* Set receive buffer */
      Cb.app_buf    = buf;
      Cb.block_num  = block_num;
      Cb.block_size = block_size * 2U;
      Cb.block_idx  = 0U;
      break;
    default:
      return AUDIO_DRV_ERROR_PARAMETER;
  }
  return AUDIO_DRV_OK;
}

/**
  \brief       Control Audio Interface.
  \param[in]   control Operation
  \return      return code
*/
int32_t AudioDrv_Control (uint32_t control) {
  int32_t err;

  if (Cb.initialized == 0U) {
    return AUDIO_DRV_ERROR;
  }

  if ((control & AUDIO_DRV_CONTROL_RX_ENABLE) != 0U) {
    free_buf ();
    Cb.dma_buf = malloc (Cb.block_size);
    if (Cb.dma_buf == NULL) {
      return AUDIO_DRV_ERROR;
    }
    Cb.count  = 0U;

    Cb.locked = 1U;
    err = BSP_AUDIO_IN_Record (0, Cb.dma_buf, Cb.block_size);
    Cb.locked = 0U;
    return err_to_audio (err);
  }
  if ((control & AUDIO_DRV_CONTROL_RX_DISABLE) != 0U) {
    err = BSP_AUDIO_IN_Stop (0);
    free_buf ();
    return err_to_audio (err);
  }
  return AUDIO_DRV_ERROR_UNSUPPORTED;
}

/**
  \brief       Get transmitted block count.
  \return      number of transmitted blocks
*/
uint32_t AudioDrv_GetTxCount (void) {
  return (0U);
}

/**
  \brief       Get received block count.
  \return      number of received blocks
*/
uint32_t AudioDrv_GetRxCount (void) {
  return (Cb.count);
}

/**
  \brief       Get Audio Interface status.
  \return      \ref AudioDrv_Status_t
*/
AudioDrv_Status_t AudioDrv_GetStatus (void) {
  AudioDrv_Status_t status;
  uint32_t bsp_state;

  BSP_AUDIO_IN_GetState (0U, &bsp_state);

  status.tx_active = 0U;
  status.rx_active = 0U;
  if (bsp_state == AUDIO_IN_STATE_RECORDING) {
    status.rx_active = 1U;
  }
  
  return (status);
}

/**
  \brief       Manage the BSP audio in half transfer complete event.
  \param[in]   Instance Audio in instance.
  \return      None.
*/
void BSP_AUDIO_IN_HalfTransfer_CallBack (uint32_t Instance) {
  uint8_t *rx_buf;
  (void)Instance;

  /* Copy recorded 1st half block to application buffer */
  rx_buf = &Cb.app_buf[Cb.block_size * Cb.block_idx];
  memcpy (&rx_buf[0], &Cb.dma_buf[0], Cb.block_size/2);
}

/**
  \brief       Manage the BSP audio in transfer complete event.
  \param[in]   Instance Audio in instance.
  \return      None.
*/
void BSP_AUDIO_IN_TransferComplete_CallBack (uint32_t Instance) {
  uint8_t *rx_buf;
  (void)Instance;

  if (Cb.locked != 0U) {
    /* Ignore immediate callback on start recording,   */
    /* first valid callback is half-transfer callback. */
    return;
  }

  /* Copy recorded 2nd half block to application buffer */
  rx_buf = &Cb.app_buf[Cb.block_size * Cb.block_idx];
  memcpy (&rx_buf[Cb.block_size/2], &Cb.dma_buf[Cb.block_size/2], Cb.block_size/2);

  if (++Cb.block_idx == Cb.block_num) {
    Cb.block_idx = 0U;
  }

  /* Increment receiver block count */
  Cb.count++;

  /* Call application callback function */
  if (Cb.callback != NULL) {
    Cb.callback (AUDIO_DRV_EVENT_RX_DATA);
  }
}
