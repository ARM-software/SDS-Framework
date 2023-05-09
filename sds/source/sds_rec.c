/*
 * Copyright (c) 2022-2023 Arm Limited. All rights reserved.
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

// SDS Recorder

#include <stdatomic.h>
#include <string.h>

#include "cmsis_compiler.h"
#include "sds.h"
#include "sdsio.h"
#include "sds_rec.h"
#include "cmsis_os2.h"

// Configuration
#ifndef SDS_REC_MAX_STREAMS
#define SDS_REC_MAX_STREAMS     8U
#endif
#ifndef SDS_REC_MAX_RECORD_SIZE
#define SDS_REC_MAX_RECORD_SIZE 8192U
#endif

#if SDS_REC_MAX_STREAMS > 31
#error "Maximum number of SDS Recorder streams is 31!"
#endif

// Control block
typedef struct {
           uint32_t    buf_size;
           uint32_t    flag_mask;
           sdsId_t     stream;
           sdsioId_t   sdsio;
  volatile uint32_t    cnt_in;
  volatile uint32_t    cnt_out;
} sdsRec_t;

static sdsRec_t   RecStreams[SDS_REC_MAX_STREAMS] = {0};
static sdsRec_t *pRecStreams[SDS_REC_MAX_STREAMS] = {NULL};

// Record header
typedef struct {
  uint32_t    timestamp;        // Timestamp in ticks
  uint32_t    data_size;        // Data size in bytes
} RecHead_t;

// Record buffer
static uint8_t RecBuf[SDS_REC_MAX_RECORD_SIZE];

// Event callback
static sdsRecEvent_t sdsRecEvent = NULL;

// Thread Id
static osThreadId_t sdsRecThreadId;

// Close event flags
static osEventFlagsId_t sdsRecCloseEventFlags;

// Event definitions
#define SDS_REC_EVENT_FLAG_MASK  ((1UL << SDS_REC_MAX_STREAMS) - 1)

#define FLAG_MASK_CLOSE (1UL << 31)

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

static sdsRec_t * sdsRecAlloc (uint32_t *index) {
  sdsRec_t *rec = NULL;
  uint32_t n;

  for (n = 0U; n < SDS_REC_MAX_STREAMS; n++) {
    if (atomic_wr32_if_zero((uint32_t *)&pRecStreams[n], (uint32_t)&RecStreams[n]) != 0U) {
      rec = &RecStreams[n];
      if (index != NULL) {
        *index = n;
      }
      break;
    }
  }
  return rec;
}

static void sdsRecFree (uint32_t index) {
  pRecStreams[index] = NULL;
}

// Event callback
static void sdsRecEventCallback (sdsId_t id, uint32_t event, void *arg) {
  sdsRec_t *rec;
  uint32_t  index = (uint32_t)arg;
  (void)id;
  (void)event;

  rec = pRecStreams[index];
  if (rec != NULL) {
    rec->flag_mask |= (1U << index);
  }
}

// Recorder thread
static __NO_RETURN void sdsRecThread (void *arg) {
  sdsRec_t *rec;
  uint32_t mask, flags, fm, cnt, n;
  RecHead_t rec_head;

  (void)arg;

  while (1) {
    flags = osThreadFlagsWait(SDS_REC_EVENT_FLAG_MASK, osFlagsWaitAny, osWaitForever);
    if ((flags & osFlagsError) == 0U) {
      for (n = 0U; n < SDS_REC_MAX_STREAMS; n++) {
        mask = (1U << n);
        if ((flags & mask) == 0U) {
          continue;
        }
        rec = pRecStreams[n];
        if (rec == NULL) {
          continue;
        }
        fm = rec->flag_mask;
        if (fm == FLAG_MASK_CLOSE) {
          continue;
        }
        while (rec->cnt_out != rec->cnt_in) {
          cnt = sdsRead(rec->stream, &rec_head, sizeof(RecHead_t));
          if (cnt == sizeof(RecHead_t)) {
            memcpy(RecBuf, &rec_head, sizeof(RecHead_t));
            cnt = sdsRead(rec->stream, RecBuf + sizeof(RecHead_t), rec_head.data_size);
            rec->cnt_out++;
            if (cnt == rec_head.data_size) {
              cnt += sizeof(RecHead_t);
              if (sdsioWrite(rec->sdsio, RecBuf, cnt) != cnt) {
                if (sdsRecEvent != NULL) {
                  sdsRecEvent(rec, SDS_REC_EVENT_IO_ERROR);
                }
              }
            }
          }
        }
        if ((fm & FLAG_MASK_CLOSE) != 0U) {
          rec->flag_mask = FLAG_MASK_CLOSE;
          osEventFlagsSet(sdsRecCloseEventFlags, mask);
        }
      }
    }
  }
}

// SDS Recorder functions

// Initialize recorder
int32_t sdsRecInit (sdsRecEvent_t event_cb) {
  int32_t ret = SDS_REC_ERROR;

  memset(pRecStreams, 0, sizeof(pRecStreams));

  if (sdsioInit() == SDSIO_OK) {
    sdsRecThreadId = osThreadNew(sdsRecThread, NULL, NULL);
    if (sdsRecThreadId != NULL)  {
      sdsRecCloseEventFlags = osEventFlagsNew(NULL);
      if (sdsRecCloseEventFlags != NULL) {
        sdsRecEvent = event_cb;
        ret = SDS_OK;
      }
    }
  }
  return ret;
}

// Uninitialize recorder
int32_t sdsRecUninit (void) {
  int32_t ret = SDS_ERROR;

  osThreadTerminate(sdsRecThreadId);
  osEventFlagsDelete(sdsRecCloseEventFlags);
  sdsRecEvent = NULL;
  sdsioUninit();

  return ret;
}

// Open recorder stream
sdsRecId_t sdsRecOpen (const char *name, void *buf, uint32_t buf_size, uint32_t io_threshold) {
  sdsRec_t *rec = NULL;
  uint32_t index;

  if ((name != NULL) && (buf != NULL) && (buf_size != 0U) &&
      (buf_size <= SDS_REC_MAX_RECORD_SIZE) && (io_threshold <= buf_size)) {

    rec = sdsRecAlloc(&index);
    if (rec != NULL) {
      rec->buf_size  = buf_size;
      rec->cnt_in    = 0U;
      rec->cnt_out   = 0U;
      rec->flag_mask = 0U;
      rec->stream    = sdsOpen(buf, buf_size, 0U, io_threshold);
      rec->sdsio     = sdsioOpen(name, sdsioModeWrite);

      if (rec->stream != NULL) {
        sdsRegisterEvents(rec->stream, sdsRecEventCallback, SDS_EVENT_DATA_HIGH, (void *)index);
      }
      if ((rec->stream == NULL) || (rec->sdsio == NULL)) {
        if (rec->stream != NULL) {
          sdsClose(rec->stream);
          rec->stream = NULL;
        }
        if (rec->sdsio != NULL) {
          sdsioClose(rec->sdsio);
          rec->sdsio = NULL;
        }
        sdsRecFree(index);
        rec = NULL;
      }
    }
  }
  return rec;
}

// Close recorder stream
int32_t sdsRecClose (sdsRecId_t id) {
  sdsRec_t *rec = id;
  uint32_t  n, mask;
  int32_t   ret = SDS_ERROR;

  mask = 0U;
  if (rec != NULL) {
    for (n = 0U; n < SDS_REC_MAX_STREAMS; n++) {
      if (pRecStreams[n] == rec) {
        mask = (1U << n);
        break;
      }
    }

    if (mask != 0U) {
      rec->flag_mask = FLAG_MASK_CLOSE | mask;
      osThreadFlagsSet(sdsRecThreadId, mask);
      osEventFlagsWait(sdsRecCloseEventFlags, mask, osFlagsWaitAll, osWaitForever);

      sdsClose(rec->stream);
      sdsioClose(rec->sdsio);
      sdsRecFree(n);

      ret = SDS_OK;
    }
  }
  return ret;
}

// Write data to recorder stream
uint32_t sdsRecWrite (sdsRecId_t id, uint32_t timestamp, const void *buf, uint32_t buf_size) {
  sdsRec_t *rec = id;
  RecHead_t rec_head;
  uint32_t  mask;
  uint32_t  num = 0U;

  if ((rec != NULL) && (buf != NULL) && (buf_size != 0U)) {
    if ((buf_size + sizeof(RecHead_t)) <= (rec->buf_size -  sdsGetCount(rec->stream))) {
      // Write record to the stream: timestamp, data size, data
      rec_head.timestamp = timestamp;
      rec_head.data_size = buf_size;
      if (sdsWrite(rec->stream, &rec_head, sizeof(RecHead_t)) == sizeof(RecHead_t)) {
        num = sdsWrite(rec->stream, buf, buf_size);
        rec->cnt_in++;
        if (num == buf_size) {
          mask = rec->flag_mask & ~FLAG_MASK_CLOSE;
          if (mask != 0U) {
            rec->flag_mask &= ~mask;
            osThreadFlagsSet(sdsRecThreadId, mask);
          }
        }
      }
    }
  }
  return num;
}
