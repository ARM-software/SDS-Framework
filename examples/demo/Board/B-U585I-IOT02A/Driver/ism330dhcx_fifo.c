/*
 * Copyright (c) 2023 Arm Limited. All rights reserved.
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

// ISM330DHCX FIFO driver

#include <stdio.h>

#include "ism330dhcx.h"
#include "ism330dhcx_fifo.h"

#ifndef BUF_SIZE_ACCELEROMETER
#define BUF_SIZE_ACCELEROMETER 1024U    // must be 2^n
#endif
#ifndef BUF_SIZE_GYROSCOPE
#define BUF_SIZE_GYROSCOPE     1024U    // must be 2^n
#endif

#ifndef BUF_SIZE_FIFO
#define BUF_SIZE_FIFO          2048U
#endif

#define SAMPLE_SIZE            6U

extern ISM330DHCX_Object_t ISM330DHCX_Obj;

/// Buffer control Block
typedef struct {
  uint8_t *buf;
  uint32_t size;
} const ism330dhcx_buf_cb_t;

/// Control Block
typedef struct {
  ism330dhcx_buf_cb_t *buf_cb;
  uint32_t             head;
  uint32_t             tail;
} ism330dhcx_cb_t;

static uint8_t buf_gyroscope[BUF_SIZE_GYROSCOPE];
static uint8_t buf_accelerometer[BUF_SIZE_ACCELEROMETER];
static uint8_t buf_fifo[BUF_SIZE_FIFO];

static ism330dhcx_buf_cb_t ism330dhcx_buf_cb[2] = {{buf_gyroscope,     sizeof(buf_gyroscope)},
                                                   {buf_accelerometer, sizeof(buf_accelerometer)}};
static ism330dhcx_cb_t ism330dhcx_cb[2] = {0};

// Write data to buffer
static uint32_t Buffer_Write (ism330dhcx_cb_t *cb, uint8_t *buf, uint32_t buf_size) {
  uint32_t cnt, cnt_limit, idx;

  cnt = cb->buf_cb->size - (cb->head - cb->tail);

  if (buf_size < cnt) {
    cnt = buf_size;
  }

  idx = cb->head & (cb->buf_cb->size -1);
  cnt_limit = cb->buf_cb->size - idx;
  if (cnt > cnt_limit) {
    // buffer rollover
    memcpy(cb->buf_cb->buf + idx, buf, cnt_limit);
    memcpy(cb->buf_cb->buf, buf + cnt_limit, cnt - cnt_limit);
  } else {
    memcpy(cb->buf_cb->buf + idx, buf, cnt);
  }
  cb->head += cnt;

  return cnt;
}

// Read data from buffer
static uint32_t Buffer_Read (ism330dhcx_cb_t *cb, uint8_t *buf, uint32_t buf_size) {
  uint32_t cnt, cnt_limit, idx;

  cnt = cb->head - cb->tail;

  if (buf_size < cnt) {
    cnt = buf_size;
  }

  idx = cb->tail & (cb->buf_cb->size -1);
  cnt_limit = cb->buf_cb->size - idx;
  if (cnt > cnt_limit) {
    // buffer rollover
    memcpy(buf, cb->buf_cb->buf + idx, cnt_limit);
    memcpy(buf + cnt_limit, cb->buf_cb->buf, cnt - cnt_limit);
  } else {
    memcpy(buf, cb->buf_cb->buf + idx, cnt);
  }
  cb->tail += cnt;

  return cnt;
}

// FIFO initialize
int32_t ISM330DHCX_FIFO_Init (uint32_t id) {
  ism330dhcx_cb_t *cb;
  int32_t          ret = -1;

  if ((id == ISM330DHCX_ID_GYROSCOPE) || (id == ISM330DHCX_ID_ACCELEROMETER)) {
    cb = &ism330dhcx_cb[id];

    cb->buf_cb = &ism330dhcx_buf_cb[id];
    cb->head   = 0U;
    cb->tail   = 0U;

    ret = 0;
  }
  return ret;
}

// FIFO uninitialize
int32_t ISM330DHCX_FIFO_Uninit (uint32_t id) {
  ism330dhcx_cb_t *cb;
  int32_t          ret = -1;

  if ((id == ISM330DHCX_ID_GYROSCOPE) || (id == ISM330DHCX_ID_ACCELEROMETER)) {
    cb = &ism330dhcx_cb[id];
    memset(cb, 0, sizeof(ism330dhcx_cb_t));
    ret = 0;
  }
  return ret;
}

// FIFO read
uint32_t ISM330DHCX_FIFO_Read (uint32_t id, uint32_t num_samples, uint8_t *buf) {
  uint32_t         idx;
  uint16_t         cnt;
  uint8_t          tag, status;
  ism330dhcx_cb_t *cb;
  stmdev_ctx_t    *ctx;
  int32_t          err = 0;
  uint32_t         num = 0U;


  // Parameter checking
  if ((id != ISM330DHCX_ID_GYROSCOPE) && (id != ISM330DHCX_ID_ACCELEROMETER)) {
    err = -1;
  }
  if ((buf == NULL) || (num_samples == 0U)) {
    err = -1;
  }  
  cb = &ism330dhcx_cb[id];
  if (cb->buf_cb == NULL) {
    err = -1;
  }

  
  if (err == 0) {
    num = Buffer_Read(cb, buf, num_samples * SAMPLE_SIZE) / SAMPLE_SIZE;

    if (num < num_samples) {

      // Get FIFO overrun status
      ctx = &ISM330DHCX_Obj.Ctx;
      if (ctx->read_reg(ctx->handle, ISM330DHCX_FIFO_STATUS2, &status, 1) == 0) {
        if (((status >> 3) & 1U) == 1U) {
//          printf("ERROR: FIFO overrun\r\n");
        }
      }

      if (ISM330DHCX_FIFO_Get_Num_Samples(&ISM330DHCX_Obj, &cnt) == 0) {
        // FIFO WORD = 7bytes = tag + sample(6bytes)
        cnt *= 7U;
        if (cnt > sizeof(buf_fifo)) {
          cnt = sizeof(buf_fifo);
          // cnt must be multiple of 7
          cnt = (cnt / 7) * 7;
        }
        ctx = &ISM330DHCX_Obj.Ctx;
        if (ctx->read_reg(ctx->handle, ISM330DHCX_FIFO_DATA_OUT_TAG, buf_fifo, cnt) == 0) {
          for (idx = 0U; idx < cnt; idx += 7) {
            tag = buf_fifo[idx] >> 3;
            if (tag == ISM330DHCX_TAG(id)) {
              memcpy(buf + (num * SAMPLE_SIZE), &buf_fifo[idx+1], SAMPLE_SIZE);
              num++;
            } else {
              switch (tag) {
                case 1:
                  // Gyroscope
                  if (ism330dhcx_cb[ISM330DHCX_ID_GYROSCOPE].buf_cb != NULL) {
                    if (Buffer_Write(&ism330dhcx_cb[ISM330DHCX_ID_GYROSCOPE], &buf_fifo[idx+1], SAMPLE_SIZE) != SAMPLE_SIZE) {
                      // Sample lost
//                      printf("ERROR: Gyroscope buffer overflow\r\n");
                    }
                  }
                  break;
                case 2:
                  // Accelerometer
                  if (ism330dhcx_cb[ISM330DHCX_ID_ACCELEROMETER].buf_cb != NULL) {
                    if (Buffer_Write(&ism330dhcx_cb[ISM330DHCX_ID_ACCELEROMETER], &buf_fifo[idx+1], SAMPLE_SIZE) != SAMPLE_SIZE) {
                      //  Sample lost
//                      printf("ERROR: Accelerometer buffer overflow\r\n");
                    }
                  }
                  break;
              }
            }
          }
        }
      }
    }
  }
  return num;
}
