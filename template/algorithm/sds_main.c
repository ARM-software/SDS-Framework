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
#include "cmsis_os2.h"
#include "sds_main.h"
#include "sds_control.h"
#include "sds_algorithm_config.h"
#include "sds_algorithm.h"
#include "sds_data_in.h"


#ifdef SDS_PLAY
// Playback record timestamp
extern uint32_t playTimestamp;
#endif

// Algorithm input/output data buffer
static uint8_t sds_algo_data_in_buf [SDS_ALGO_DATA_IN_BLOCK_SIZE]  __ALIGNED(4);
static uint8_t sds_algo_data_out_buf[SDS_ALGO_DATA_OUT_BLOCK_SIZE] __ALIGNED(4);

// SDS buffers
#ifdef SDS_PLAY
static uint8_t sds_play_buf_data_in[((SDS_ALGO_DATA_IN_BLOCK_SIZE  * 2) + 2048)] __ALIGNED(4);
#else
static uint8_t sds_rec_buf_data_in [((SDS_ALGO_DATA_IN_BLOCK_SIZE  * 2) + 2048)] __ALIGNED(4);
#endif
static uint8_t sds_rec_buf_data_out[((SDS_ALGO_DATA_OUT_BLOCK_SIZE * 2) + 2048)] __ALIGNED(4);

// SDS identifiers
#ifdef SDS_PLAY
       sdsRecPlayId_t playIdDataInput = NULL;
#else
static sdsRecPlayId_t recIdDataInput  = NULL;
#endif
static sdsRecPlayId_t recIdDataOutput = NULL;


// Public functions

/**
  \fn           int32_t OpenStreams (void)
  \brief        Open streams used by the application.
  \return       0 on success; -1 on error
*/
int32_t OpenStreams (void) {
  int32_t status = 0;

#ifdef SDS_PLAY
  // Open stream for playback of input data
  // Check https://arm-software.github.io/SDS-Framework/main/theory.html#filenames for details on playback filename
  playIdDataInput = sdsPlayOpen("DataInput", sds_play_buf_data_in, sizeof(sds_play_buf_data_in));
  SDS_ASSERT(playIdDataInput != NULL);
  if (playIdDataInput == NULL) {
    printf("Failed to open SDS stream for playback of input data!\n");
    status = -1;
  }
#else
  // Open stream for recording of input data
  recIdDataInput = sdsRecOpen("DataInput", sds_rec_buf_data_in, sizeof(sds_rec_buf_data_in));
  SDS_ASSERT(recIdDataInput != NULL);
  if (recIdDataInput == NULL) {
    printf("Failed to open SDS stream for recording of input data!\n");
    status = -1;
  }
#endif

  // Open stream for recording of output data
  recIdDataOutput = sdsRecOpen("DataOutput", sds_rec_buf_data_out, sizeof(sds_rec_buf_data_out));
  SDS_ASSERT(recIdDataOutput != NULL);
  if (recIdDataOutput == NULL) {
    printf("Failed to open SDS stream for recording of output data!\n");
    status = -1;
  }

#ifdef SDS_PLAY
  if (status == 0) {
    printf("SDS playback and recording started\n");
  } else {
    printf("SDS playback and recording start failed!\n");
    printf("For Network and USB SDSIO Interfaces ensure that SDSIO Server is running and restart the application!\n");
  }
#else
  if (status == 0) {
    printf("SDS recording started\n");
  } else {
    printf("SDS recording start failed!\n");
    printf("For Network and USB SDSIO Interfaces ensure that SDSIO Server is running and restart the application!\n");
  }
#endif

  return status;
}

/**
  \fn           int32_t CloseStreams (void)
  \brief        Close streams used by the application.
  \return       0 on success; -1 on error
*/
int32_t CloseStreams (void) {
  int32_t status = 0;

#ifdef SDS_PLAY
  // Close stream for playback of input data
  status = sdsPlayClose(playIdDataInput);
  SDS_ASSERT(status == SDS_REC_PLAY_OK);
  if (status != 0) {
    printf("Failed to close SDS stream for playback of input data!\n");
    status = -1;
  }
#else
  // Close stream for recording of input data
  status = sdsRecClose(recIdDataInput);
  SDS_ASSERT(status == SDS_REC_PLAY_OK);
  if (status != 0) {
    printf("Failed to close SDS stream for recording of input data!\n");
    status = -1;
  }
#endif

  // Close stream for recording of output data
  status = sdsRecClose(recIdDataOutput);
  SDS_ASSERT(status == SDS_REC_PLAY_OK);
  if (status != 0) {
    printf("Failed to close SDS stream for recording of output data!\n");
    status = -1;
  }

#ifdef SDS_PLAY
  if (status == 0) {
    printf("SDS playback and recording stopped\n");
  } else {
    printf("SDS playback and recording stop failed!\n");
  }
#else
  if (status == 0) {
    printf("SDS recording stopped\n");
  } else {
    printf("SDS recording stop failed!\n");
  }
#endif

  return status;
}


// Algorithm Thread function
__NO_RETURN void AlgorithmThread (void *argument) {
  uint32_t timestamp;
  int32_t  retv;
  (void)argument;

  // Initialize data acquisition
  InitInputData();

  // Initialize algorithm under test
  InitAlgorithm();

  for (;;) {
    if (sdsStreamingState == SDS_STREAMING_STOP) {
      // Request to stop streaming, transit to state safe for stopping
      sdsStreamingState = SDS_STREAMING_STOP_SAFE;
    }

    // Get a block of input data as required by algorithm under test
    if (GetInputData(sds_algo_data_in_buf, sizeof(sds_algo_data_in_buf)) != sizeof(sds_algo_data_in_buf)) {
      // If there was an error retrieving data skip algorithm execution
      continue;
    }

    // Execute algorithm under test
    if (ExecuteAlgorithm((const uint8_t *)sds_algo_data_in_buf, sizeof(sds_algo_data_in_buf), sds_algo_data_out_buf, sizeof(sds_algo_data_out_buf)) != 0) {
      // If there was an error executing algorithm skip recording
      continue;
    }

    if (sdsStreamingState == SDS_STREAMING_ACTIVE) {
#ifdef SDS_PLAY
      timestamp = playTimestamp;
#else
      timestamp = osKernelGetTickCount();

      // Record algorithm input data
      retv = sdsRecWrite(recIdDataInput, timestamp, sds_algo_data_in_buf, sizeof(sds_algo_data_in_buf));
      SDS_ASSERT(retv == sizeof(sds_algo_data_in_buf));
#endif

      // Record algorithm output data
      retv = sdsRecWrite(recIdDataOutput, timestamp, sds_algo_data_out_buf, sizeof(sds_algo_data_out_buf));
      SDS_ASSERT(retv == sizeof(sds_algo_data_out_buf));
    }
  }
}
