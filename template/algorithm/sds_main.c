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

#include "cmsis_os2.h"
#include "sds.h"
#include "sds_main.h"
#include "algorithm_config.h"
#include "algorithm.h"
#include "data_in.h"


// Algorithm input/output data buffer
static uint8_t algo_data_in_buf [ALGO_DATA_IN_BLOCK_SIZE]  __ALIGNED(4);
static uint8_t algo_data_out_buf[ALGO_DATA_OUT_BLOCK_SIZE] __ALIGNED(4);

// SDS buffers
static uint8_t sds_data_in_buf [((ALGO_DATA_IN_BLOCK_SIZE  * 2) + 2048)] __ALIGNED(4);
static uint8_t sds_data_out_buf[((ALGO_DATA_OUT_BLOCK_SIZE * 2) + 2048)] __ALIGNED(4);

// SDS identifiers
static sdsId_t sds_data_in_id  = NULL;
static sdsId_t sds_data_out_id = NULL;

// Recording/playback mode text
static const char *SDS_MODE[] = { "recording", "playback" };

// Public functions

/**
  \fn           int32_t OpenStreams (void)
  \brief        Open streams used by the application.
  \return       0 on success; -1 on error
*/
int32_t OpenStreams (void) {
  int32_t status = 0;
  uint8_t play = 0U;

  if ((sdsFlags & SDS_FLAG_PLAYBACK) != 0U) {   // If open for playback requested
    play = 1U;

    // Reset algorithm under test before starting a playback run
    ResetAlgorithm();
  }

  // Open stream for playback or recording of input data, depending on the mode
  if (play != 0U) {                             // -- Playback
    // Check https://arm-software.github.io/SDS-Framework/main/theory.html#filenames for details on playback filename
    sds_data_in_id = sdsOpen("ML_In", sdsModeRead, sds_data_in_buf, sizeof(sds_data_in_buf));
  } else {                                      // -- Recording
    sds_data_in_id = sdsOpen("ML_In", sdsModeWrite, sds_data_in_buf, sizeof(sds_data_in_buf));
  }
  // Open stream for recording of output data
  if (sds_data_in_id != NULL) {
    sds_data_out_id = sdsOpen("ML_Out", sdsModeWrite, sds_data_out_buf, sizeof(sds_data_out_buf));
  }

  SDS_ASSERT(sds_data_in_id  != NULL);
  SDS_ASSERT(sds_data_out_id != NULL);

  if ((sds_data_in_id != NULL) && (sds_data_out_id != NULL)) {
    SDS_PRINTF("==== SDS %s started\n", SDS_MODE[play]);
  } else {
    sdsState = SDS_STATE_END;       // If files could not be opened then request streaming end
    status = -1;
    SDS_PRINTF("==== SDS %s start failed\n", SDS_MODE[play]);
  }

  return status;
}

/**
  \fn           int32_t CloseStreams (void)
  \brief        Close streams used by the application.
  \return       0 on success; -1 on error
*/
int32_t CloseStreams (void) {
  int32_t close_status;
  int32_t status = 0;
  uint8_t play = 0U;

  if ((sdsFlags & SDS_FLAG_PLAYBACK) != 0U) {   // If open for playback requested
    play = 1U;
  }

  close_status = sdsClose(sds_data_in_id);
  SDS_ERROR_CHECK(close_status);
  if (close_status == SDS_OK) {
    close_status = sdsClose(sds_data_out_id);
    SDS_ERROR_CHECK(close_status);
  }

  if (close_status == SDS_OK) {
    SDS_PRINTF("==== SDS %s stopped\n", SDS_MODE[play]);
  } else {
    status = -1;
    SDS_PRINTF("==== SDS %s stop failed\n", SDS_MODE[play]);
  }

  return status;
}


// Algorithm Thread function
__NO_RETURN void AlgorithmThread (void *argument) {
  uint32_t sds_state, sds_flags;
  uint32_t timeslot;
  int32_t  ret;
  (void)argument;

  // Initialize data acquisition
  InitInputData();

  // Initialize algorithm under test
  InitAlgorithm();

  for (;;) {
    sds_state = sdsState;
    sds_flags = sdsFlags;

    if ((sds_state == SDS_STATE_ACTIVE) && ((sds_flags & SDS_FLAG_START) == 0U)) {
      // If start flag was cleared during active streaming, request streaming stop
      sdsState = SDS_STATE_STOP_REQ;
      continue;
    }

    if ((sds_flags & SDS_FLAG_PLAYBACK) != 0U) {        // -- Playback
      // Discard input data during playback
      DiscardInputData();

      // Wait for playback activation
      if (sds_state != SDS_STATE_ACTIVE) {
        osDelay(100U);
        DiscardInputData();
        continue;
      }

      // Read input data from playback stream
      do {
        ret = sdsRead(sds_data_in_id, &timeslot, algo_data_in_buf, sizeof(algo_data_in_buf));
        if (ret == SDS_NO_DATA) {
          osDelay(10U);
          DiscardInputData();
        }
      } while (ret == SDS_NO_DATA);
    
      if (ret > 0) {
        SDS_ASSERT(ret == sizeof(algo_data_in_buf));
      } else {
        // If there is no more data for playback, request streaming stop
        sdsState = SDS_STATE_STOP_REQ;
        continue;
      }
    } else {                                            // -- Recording
      // Get a block of input data as required by algorithm under test
      if (GetInputData(algo_data_in_buf, sizeof(algo_data_in_buf)) != sizeof(algo_data_in_buf)) {
        // If there was an error retrieving data skip algorithm execution
        continue;
      }

      if (sds_state == SDS_STATE_ACTIVE) {
        timeslot = osKernelGetTickCount();

        // Record algorithm input data
        do {
          ret = sdsWrite(sds_data_in_id, timeslot, algo_data_in_buf, sizeof(algo_data_in_buf));
          if (ret == SDS_NO_SPACE) {
            osDelay(10U);
          }
        } while (ret == SDS_NO_SPACE);
        SDS_ASSERT(ret == sizeof(algo_data_in_buf));
      }
    }

    // Execute algorithm under test
    if (ExecuteAlgorithm(algo_data_in_buf, sizeof(algo_data_in_buf), algo_data_out_buf, sizeof(algo_data_out_buf)) != 0) {
      // If there was an error executing algorithm skip recording
      continue;
    }

    if (sds_state == SDS_STATE_ACTIVE) {
      // Record algorithm output data
      ret = sdsWrite(sds_data_out_id, timeslot, algo_data_out_buf, sizeof(algo_data_out_buf));
      SDS_ASSERT(ret == sizeof(algo_data_out_buf));
    }
  }
}
