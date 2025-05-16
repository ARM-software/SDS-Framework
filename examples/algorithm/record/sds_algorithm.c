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

#include "sds_algorithm_config.h"
#include "sds_algorithm.h"

#include <stdio.h>

#include "cmsis_os2.h"
#include "sds_control.h"

// Algorithm input/output data buffer
static uint8_t sds_algo_data_in_buf [SDS_ALGO_DATA_IN_BLOCK_SIZE]  __ALIGNED(4);
static uint8_t sds_algo_data_out_buf[SDS_ALGO_DATA_OUT_BLOCK_SIZE] __ALIGNED(4);

// SDS buffers
static uint8_t sds_rec_buf_data_in [((SDS_ALGO_DATA_IN_BLOCK_SIZE  * 2) + 2048)] __ALIGNED(4);
static uint8_t sds_rec_buf_data_out[((SDS_ALGO_DATA_OUT_BLOCK_SIZE * 2) + 2048)] __ALIGNED(4);

// SDS identifiers
static sdsRecPlayId_t recIdDataInput  = NULL;
static sdsRecPlayId_t recIdDataOutput = NULL;


// Public functions

/**
  \fn           int32_t OpenStreams (void)
  \brief        Open streams used by the application.
  \return       0 on success; -1 on error
*/
int32_t OpenStreams (void) {
  // Open stream for recording of input data
  recIdDataInput = sdsRecOpen("DataInput", sds_rec_buf_data_in, sizeof(sds_rec_buf_data_in));
  SDS_ASSERT(recIdDataInput != NULL);
  if (recIdDataInput == NULL) {
    printf("Failed to open SDS stream for recording of input data\n");
  }

  // Open stream for recording of output data
  recIdDataOutput = sdsRecOpen("DataOutput", sds_rec_buf_data_out, sizeof(sds_rec_buf_data_out));
  SDS_ASSERT(recIdDataOutput != NULL);
  if (recIdDataOutput == NULL) {
    printf("Failed to open SDS stream for recording of output data\n");
  }

  printf("SDS recording started\n");
  return 0;
}

/**
  \fn           int32_t CloseStreams (void)
  \brief        Close streams used by the application.
  \return       0 on success; -1 on error
*/
int32_t CloseStreams (void) {
  int32_t status;

  // Close stream for recording of input data
  status = sdsRecClose(recIdDataInput);
  SDS_ASSERT(status == SDS_REC_PLAY_OK);
  if (status != 0) {
    printf("Failed to close SDS stream for recording of input data\n");
    return -1;
  }

  // Close stream for recording of output data
  status = sdsRecClose(recIdDataOutput);
  SDS_ASSERT(status == SDS_REC_PLAY_OK);
  if (status != 0) {
    printf("Failed to close SDS stream for recording of output data\n");
    return -1;
  }

  printf("SDS recording stopped\n");
  return 0;
}


// Algorithm Thread function
__NO_RETURN void AlgorithmThread (void *argument) {
  uint32_t timestamp;
  int32_t  retv;
  (void)argument;

  // Initialize data acquisition
#warning "Replace this line with code to initialize data acquisition!"

  // Initialize algorithm under test
#warning "Replace this line with code to initialize algorithm under test!"

  for (;;) {
    if (sdsStreamingState == SDS_STREAMING_STOP) {
      // Request to stop streaming, transit to state safe for stopping
      sdsStreamingState = SDS_STREAMING_STOP_SAFE;
    }

    // Get a block of input data as required by algorithm under test
#warning "Replace this and following line with code to get a block of input data as required by algorithm under test into sds_algo_data_in_buf!"
    osDelay(100U);

    // Execute algorithm under test
#warning "Replace this line with code to execute algorithm under test using sds_algo_data_in_buf as input and sds_algo_data_out_buf as output!"

    if (sdsStreamingState == SDS_STREAMING_ACTIVE) {
      timestamp = osKernelGetTickCount();

      // Record algorithm input data
      retv = sdsRecWrite(recIdDataInput, timestamp, sds_algo_data_in_buf, sizeof(sds_algo_data_in_buf));
      SDS_ASSERT(retv == sizeof(sds_algo_data_in_buf));

      // Record algorithm output data
      retv = sdsRecWrite(recIdDataOutput, timestamp, sds_algo_data_out_buf, sizeof(sds_algo_data_out_buf));
      SDS_ASSERT(retv == sizeof(sds_algo_data_out_buf));
    }
  }
}
