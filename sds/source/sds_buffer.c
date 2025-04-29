/*
 * Copyright (c) 2022-2025 Arm Limited. All rights reserved.
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

// Synchronous Data Stream (SDS) Buffer

#include <stdatomic.h>
#include <string.h>

#include "cmsis_compiler.h"
#include "sds_buffer.h"
#include "sds_buffer_config.h"

// Control block
typedef struct {
           sdsBufferEvent_t  event_cb;
           uint32_t          event_mask;
           void             *event_arg;
           uint8_t          *buf;
           uint32_t          buf_size;
           uint32_t          threshold_high;
           uint32_t          threshold_low;
  volatile uint32_t          cnt_in;
  volatile uint32_t          cnt_out;
  volatile uint32_t          idx_in;
  volatile uint32_t          idx_out;
} sdsBuffer_t;

// Allocate memory for the SDS Buffer streams depending on configured maximum number of streams.
static sdsBuffer_t   SDSBufferStreams[SDS_BUFFER_MAX_STREAMS] = {0};
static sdsBuffer_t *pSDSBufferStreams[SDS_BUFFER_MAX_STREAMS] = {NULL};

// Helper functions

// Atomic Operation: Write 32-bit value to memory, if existing value in memory is zero
//  Return: 1 when new value is written or 0 otherwise
#if ATOMIC_CHAR32_T_LOCK_FREE < 2
__STATIC_INLINE uint32_t atomic_wr32_if_zero (uint32_t *mem, uint32_t val) {
  uint32_t primask = __get_PRIMASK();
  uint32_t ret = 0U;

  __disable_irq();
  if (*mem == 0U) {
    *mem = val;
    ret = 1U;
  }
  if (primask == 0U) {
    __enable_irq();
  }

  return ret;
}
#else
__STATIC_INLINE uint32_t atomic_wr32_if_zero (uint32_t *mem, uint32_t val) {
  uint32_t expected;
  uint32_t ret = 1U;

  expected = *mem;
  do {
    if (expected != 0U) {
      ret = 0U;
      break;
    }
  } while (!atomic_compare_exchange_weak_explicit((_Atomic uint32_t *)mem,
                                                  &expected,
                                                  val,
                                                  memory_order_acq_rel,
                                                  memory_order_relaxed));

  return ret;
}
#endif

static sdsBuffer_t *sdsBufferAlloc (void) {
  sdsBuffer_t *sds_buffer = NULL;
  uint32_t     n;

  for (n = 0U; n < SDS_BUFFER_MAX_STREAMS; n++) {
    if (atomic_wr32_if_zero((uint32_t *)&pSDSBufferStreams[n], (uint32_t)&SDSBufferStreams[n]) != 0U) {
      sds_buffer = &SDSBufferStreams[n];
      break;
    }
  }
  return sds_buffer;
}

static void sdsBufferFree (sdsBuffer_t *sds_buffer) {
  uint32_t n;

  if (sds_buffer != NULL) {
    for (n = 0U; n < SDS_BUFFER_MAX_STREAMS; n++) {
      if (pSDSBufferStreams[n] == sds_buffer) {
        pSDSBufferStreams[n] = NULL;
        break;
      }
    }
  }
}

// Open SDS Buffer stream
sdsBufferId_t sdsBufferOpen (void *buf, uint32_t buf_size, uint32_t threshold_low, uint32_t threshold_high) {
  sdsBuffer_t *sds_buffer = NULL;

  // Buffer pointer needs to be valid
  if ((buf != NULL) && (buf_size != 0U)) {
    sds_buffer = sdsBufferAlloc();
    if (sds_buffer != NULL) {
      memset(sds_buffer, 0, sizeof(sdsBuffer_t));
      sds_buffer->buf            = buf;
      sds_buffer->buf_size       = buf_size;
      sds_buffer->threshold_low  = threshold_low;
      sds_buffer->threshold_high = threshold_high;
    }
  }
  return sds_buffer;
}

// Close SDS Buffer stream
int32_t sdsBufferClose (sdsBufferId_t id) {
  sdsBuffer_t *sds_buffer = id;
  int32_t      ret = SDS_BUFFER_ERROR_PARAMETER;

  if (sds_buffer != NULL) {
    sdsBufferFree(sds_buffer);
    ret = SDS_BUFFER_OK;
  }
  return ret;
}

// Register SDS Buffer stream events
int32_t sdsBufferRegisterEvents (sdsBufferId_t id, sdsBufferEvent_t event_cb, uint32_t event_mask, void *event_arg) {
  sdsBuffer_t *sds_buffer = id;
  int32_t      ret = SDS_BUFFER_ERROR_PARAMETER;

  if (sds_buffer != NULL) {
    sds_buffer->event_cb   = event_cb;
    sds_buffer->event_mask = event_mask;
    sds_buffer->event_arg  = event_arg;
    ret = SDS_BUFFER_OK;
  }
  return ret;
}

// Write data to SDS Buffer stream
int32_t sdsBufferWrite (sdsBufferId_t id, const void *buf, uint32_t buf_size) {
  sdsBuffer_t   *sds_buffer = id;
  uint32_t       num = 0U;
  uint32_t       cnt_free, cnt_used, cnt_limit;
  int32_t        ret = SDS_BUFFER_ERROR_PARAMETER;

  if ((sds_buffer != NULL) && (buf != NULL) && (buf_size != 0U)) {

    cnt_used = sds_buffer->cnt_in - sds_buffer->cnt_out;
    cnt_free = sds_buffer->buf_size - cnt_used;

    if (buf_size < cnt_free) {
      num = buf_size;
    } else {
      // not enough space in buffer
      num = cnt_free;
    }

    cnt_limit = sds_buffer->buf_size - sds_buffer->idx_in;
    if (num > cnt_limit) {
      // buffer rollover
      memcpy(sds_buffer->buf + sds_buffer->idx_in, buf, cnt_limit);
      memcpy(sds_buffer->buf, (const uint8_t *)buf + cnt_limit, num - cnt_limit);
      sds_buffer->idx_in = num - cnt_limit;
    } else {
      memcpy(sds_buffer->buf + sds_buffer->idx_in, buf, num);
      sds_buffer->idx_in += num;
    }
    sds_buffer->cnt_in += num;

    if ((sds_buffer->event_cb != NULL) && (sds_buffer->event_mask & SDS_BUFFER_EVENT_DATA_HIGH)) {
      cnt_used = sds_buffer->cnt_in - sds_buffer->cnt_out;
      if (cnt_used >= sds_buffer->threshold_high) {
        sds_buffer->event_cb(sds_buffer, SDS_BUFFER_EVENT_DATA_HIGH, sds_buffer->event_arg);
      }
    }
    ret = (int32_t)num;
  }
  return ret;
}

// Read data from SDS Buffer stream
int32_t sdsBufferRead (sdsBufferId_t id, void *buf, uint32_t buf_size) {
  sdsBuffer_t *sds_buffer = id;
  uint32_t     num = 0U;
  uint32_t     cnt_used, cnt_limit;
  int32_t      ret = SDS_BUFFER_ERROR_PARAMETER;

  if ((sds_buffer != NULL) && (buf != NULL) && (buf_size != 0U)) {

    cnt_used = sds_buffer->cnt_in - sds_buffer->cnt_out;

    if (buf_size < cnt_used) {
      num = buf_size;
    } else {
      // not enough data available
      num = cnt_used;
    }

    cnt_limit = sds_buffer->buf_size - sds_buffer->idx_out;
    if (num > cnt_limit) {
      // buffer rollover
      memcpy(buf, sds_buffer->buf + sds_buffer->idx_out, cnt_limit);
      memcpy((uint8_t *)buf + cnt_limit, sds_buffer->buf, num - cnt_limit);
      sds_buffer->idx_out = num - cnt_limit;
    } else {
      memcpy(buf, sds_buffer->buf + sds_buffer->idx_out, num);
      sds_buffer->idx_out += num;
    }
    sds_buffer->cnt_out += num;

    if ((sds_buffer->event_cb != NULL) && (sds_buffer->event_mask & SDS_BUFFER_EVENT_DATA_LOW)) {
      cnt_used = sds_buffer->cnt_in - sds_buffer->cnt_out;
      if (cnt_used <= sds_buffer->threshold_low) {
        sds_buffer->event_cb(sds_buffer, SDS_BUFFER_EVENT_DATA_LOW, sds_buffer->event_arg);
      }
    }

    ret = (int32_t)num;
  }
  return ret;
}

// Clear SDS Buffer stream data
int32_t sdsBufferClear (sdsBufferId_t id) {
  sdsBuffer_t *sds_buffer = id;
  uint32_t     cnt_used, cnt_limit;
  int32_t      ret = SDS_BUFFER_ERROR_PARAMETER;

  if (sds_buffer != NULL) {
    cnt_used = sds_buffer->cnt_in - sds_buffer->cnt_out;
    cnt_limit = sds_buffer->buf_size - sds_buffer->idx_out;
    if (cnt_used > cnt_limit) {
      // buffer rollover
      sds_buffer->idx_out = cnt_used - cnt_limit;
    } else {
      sds_buffer->idx_out += cnt_used;
    }
    sds_buffer->cnt_out += cnt_used;

    ret = SDS_BUFFER_OK;
  }
  return ret;
}

// Get data count in SDS Buffer stream
int32_t sdsBufferGetCount (sdsBufferId_t id) {
  sdsBuffer_t *sds_buffer = id;
  int32_t      ret = SDS_BUFFER_ERROR_PARAMETER;

  if (sds_buffer != NULL) {
    ret = (int32_t)(sds_buffer->cnt_in - sds_buffer->cnt_out);
  }
  return ret;
}
