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

// SDS Recorder & Player

#include <stdatomic.h>
#include <string.h>

#include "cmsis_compiler.h"
#include "cmsis_os2.h"
#include "sds.h"
#include "sdsio.h"
#include "sds_rec_play.h"
#include "sds_rec_play_config.h"

#if SDS_REC_PLAY_MAX_STREAMS > 31
#error "Maximum number of SDS Recorder/Player streams is 31!"
#endif

// Record header
typedef struct {
  uint32_t    timestamp;        // Timestamp in ticks
  uint32_t    data_size;        // Data size in bytes
} recPlayHead_t;
#define HEAD_SIZE sizeof(recPlayHead_t)

// Control block
typedef struct {
           uint8_t         index;
           uint8_t         event_threshold;
           uint8_t         event_close;
           uint8_t         flags;
           uint32_t        buf_size;
           sdsId_t         stream;
           sdsioId_t       sdsio;
           recPlayHead_t   head;
} sdsRecPlay_t;

static sdsRecPlay_t   RecPlayStreams[SDS_REC_PLAY_MAX_STREAMS] = {0};
static sdsRecPlay_t *pRecPlayStreams[SDS_REC_PLAY_MAX_STREAMS] = {NULL};

// Record buffer
static uint8_t RecPlayBuf[SDS_REC_PLAY_BUF_SIZE];

// Event callback
static sdsRecPlayEvent_t sdsRecPlayEvent = NULL;

// Thread Id
static osThreadId_t sdsRecPlayThreadId;

// Close event flags
static osEventFlagsId_t sdsRecPlayCloseEventFlags;

// Event definitions
#define SDS_REC_PLAY_EVENT_FLAG_MASK ((1UL << SDS_REC_PLAY_MAX_STREAMS) - 1)

// Flag definitions
#define SDS_REC_PLAY_FLAG_REC_STREAM  (1U << 0)
#define SDS_REC_PLAY_FLAG_PLAY_STREAM (1U << 1)
#define SDS_REC_PLAY_FLAG_CLOSE       (1U << 3)
#define SDS_REC_PLAY_FLAG_EOS         (1U << 3)

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

static sdsRecPlay_t * sdsRecPlayAlloc (uint32_t *index) {
  sdsRecPlay_t *recPlay = NULL;
  uint32_t      n;

  for (n = 0U; n < SDS_REC_PLAY_MAX_STREAMS; n++) {
    if (atomic_wr32_if_zero((uint32_t *)&pRecPlayStreams[n], (uint32_t)&RecPlayStreams[n]) != 0U) {
      recPlay = &RecPlayStreams[n];
      if (index != NULL) {
        *index = n;
      }
      break;
    }
  }
  return recPlay;
}

static void sdsRecPlayFree (uint32_t index) {
  pRecPlayStreams[index] = NULL;
}

// Event callback
static void sdsRecPlayEventCallback (sdsId_t id, uint32_t event, void *arg) {
  uint32_t      index = (uint32_t)arg;
  sdsRecPlay_t *recPlay;
  (void)id;
  (void)event;

  recPlay = pRecPlayStreams[index];
  if (recPlay != NULL) {
    recPlay->event_threshold = 1U;
  }
}

// Recorder Handler
static void sdsRecHandler (sdsRecPlay_t *recPlay) {
  uint32_t cnt;

  if ((recPlay->flags & SDS_REC_PLAY_FLAG_CLOSE) != 0U) {
    return;
  }

  cnt = sdsGetCount(recPlay->stream);
  while (cnt != 0U) {
    if (cnt > sizeof(RecPlayBuf)) {
      cnt = sizeof(RecPlayBuf);
    }
    sdsRead(recPlay->stream, RecPlayBuf, cnt);
    if (sdsioWrite(recPlay->sdsio, RecPlayBuf, cnt) != cnt) {
      if (sdsRecPlayEvent != NULL) {
        sdsRecPlayEvent(recPlay, SDS_REC_PLAY_EVENT_IO_ERROR);
      }
      break;
    }
    cnt = sdsGetCount(recPlay->stream);
  }

  if (recPlay->event_close != 0U) {
    recPlay->flags |= SDS_REC_PLAY_FLAG_CLOSE;
    osEventFlagsSet(sdsRecPlayCloseEventFlags, 1U << recPlay->index);
  }
}

// Player Handler
static void sdsPlayHandler (sdsRecPlay_t *recPlay) {
  uint32_t cnt, num;

  if ((recPlay->flags & SDS_REC_PLAY_FLAG_CLOSE) != 0U) {
    return;
  }
  if (recPlay->event_close != 0U) {
    recPlay->flags |= SDS_REC_PLAY_FLAG_CLOSE;
    osEventFlagsSet(sdsRecPlayCloseEventFlags, 1U << recPlay->index);
    return;
  }

  while (1) {
    cnt = recPlay->buf_size - sdsGetCount(recPlay->stream);
    if (cnt == 0U) {
      break;
    }
    if (cnt > sizeof(RecPlayBuf)) {
      cnt = sizeof(RecPlayBuf);
    }
    num = sdsioRead(recPlay->sdsio, RecPlayBuf, cnt);
    if (num != 0U) {
      sdsWrite(recPlay->stream, RecPlayBuf, num);
    }
    if (num != cnt) {
      if (sdsioEndOfStream(recPlay->sdsio) != 0) {
        recPlay->flags |= SDS_REC_PLAY_FLAG_EOS;
      } else {
        if (sdsRecPlayEvent != NULL) {
          sdsRecPlayEvent(recPlay, SDS_REC_PLAY_EVENT_IO_ERROR);
        }
      }
      break;
    }
  }
}

// Recorder thread
static __NO_RETURN void sdsRecPlayThread (void *arg) {
  sdsRecPlay_t *recPlay;
  uint32_t      flags, n;

  (void)arg;

  while (1) {
    flags = osThreadFlagsWait(SDS_REC_PLAY_EVENT_FLAG_MASK, osFlagsWaitAny, osWaitForever);
    if ((flags & osFlagsError) == 0U) {
      for (n = 0U; n < SDS_REC_PLAY_MAX_STREAMS; n++) {
        if ((flags & (1U << n)) == 0U) {
          continue;
        }
        recPlay = pRecPlayStreams[n];
        if (recPlay == NULL) {
          continue;
        }

        if ((recPlay->flags & SDS_REC_PLAY_FLAG_REC_STREAM) != 0U) {
          sdsRecHandler(recPlay);
        } else if ((recPlay->flags & SDS_REC_PLAY_FLAG_PLAY_STREAM) != 0U) {
          sdsPlayHandler(recPlay);
        }
      }
    }
  }
}

// Initialize recorder and player
int32_t sdsRecPlayInit (sdsRecPlayEvent_t event_cb) {
  int32_t ret = SDS_REC_PLAY_ERROR;

  memset(pRecPlayStreams, 0, sizeof(pRecPlayStreams));

  if (sdsioInit() == SDSIO_OK) {
    sdsRecPlayThreadId = osThreadNew(sdsRecPlayThread, NULL, NULL);
    if (sdsRecPlayThreadId != NULL)  {
      sdsRecPlayCloseEventFlags = osEventFlagsNew(NULL);
      if (sdsRecPlayCloseEventFlags != NULL) {
        sdsRecPlayEvent = event_cb;
        ret = SDS_REC_PLAY_OK;
      }
    }
  }
  return ret;
}

// Uninitialize recorder
int32_t sdsRecPlayUninit (void) {
  int32_t ret = SDS_REC_PLAY_ERROR;

  osThreadTerminate(sdsRecPlayThreadId);
  osEventFlagsDelete(sdsRecPlayCloseEventFlags);
  sdsRecPlayEvent = NULL;
  sdsioUninit();

  return ret;
}

// SDS Recorder functions

// Open recorder stream
sdsRecPlayId_t sdsRecOpen (const char *name, void *buf, uint32_t buf_size, uint32_t io_threshold) {
  sdsRecPlay_t *recPlay = NULL;
  uint32_t      index;

  if ((name != NULL) && (buf != NULL) && (buf_size != 0U) &&
      (buf_size <= SDS_REC_PLAY_BUF_SIZE) && (io_threshold <= buf_size)) {

    recPlay = sdsRecPlayAlloc(&index);
    if (recPlay != NULL) {
      recPlay->index           = index & 0xFFU;
      recPlay->event_threshold = 0U;
      recPlay->event_close     = 0U;
      recPlay->flags           = SDS_REC_PLAY_FLAG_REC_STREAM;
      recPlay->buf_size        = buf_size;
      recPlay->stream          = sdsOpen(buf, buf_size, 0U, io_threshold);
      recPlay->sdsio           = sdsioOpen(name, sdsioModeWrite);

      if (recPlay->stream != NULL) {
        sdsRegisterEvents(recPlay->stream, sdsRecPlayEventCallback, SDS_EVENT_DATA_HIGH, (void *)index);
      }
      if ((recPlay->stream == NULL) || (recPlay->sdsio == NULL)) {
        if (recPlay->stream != NULL) {
          sdsClose(recPlay->stream);
          recPlay->stream = NULL;
        }
        if (recPlay->sdsio != NULL) {
          sdsioClose(recPlay->sdsio);
          recPlay->sdsio = NULL;
        }
        sdsRecPlayFree(index);
        recPlay = NULL;
      }
    }
  }
  return recPlay;
}

// Close recorder stream
int32_t sdsRecClose (sdsRecPlayId_t id) {
  sdsRecPlay_t *recPlay = id;
  int32_t       ret = SDS_REC_PLAY_ERROR;
  uint32_t      event_mask;

  if (recPlay != NULL) {
    event_mask =  1 << recPlay->index;
    recPlay->event_close = 1U;
    osThreadFlagsSet(sdsRecPlayThreadId, event_mask);
    osEventFlagsWait(sdsRecPlayCloseEventFlags, event_mask, osFlagsWaitAll, osWaitForever);

    sdsClose(recPlay->stream);
    sdsioClose(recPlay->sdsio);
    sdsRecPlayFree(recPlay->index);

    ret = SDS_REC_PLAY_OK;
  }
  return ret;
}

// Write data to recorder stream
uint32_t sdsRecWrite (sdsRecPlayId_t id, uint32_t timestamp, const void *buf, uint32_t buf_size) {
  sdsRecPlay_t *recPlay = id;
  uint32_t      num = 0U;
  recPlayHead_t head;

  if ((recPlay != NULL) && (buf != NULL) && (buf_size != 0U)) {
    if ((buf_size + sizeof(recPlayHead_t)) <= (recPlay->buf_size -  sdsGetCount(recPlay->stream))) {
      // Write record to the stream: timestamp, data size, data
      head.timestamp = timestamp;
      head.data_size = buf_size;
      if (sdsWrite(recPlay->stream, &head, sizeof(recPlayHead_t)) == sizeof(recPlayHead_t)) {
        num = sdsWrite(recPlay->stream, buf, buf_size);
        if (num == buf_size) {
          if (recPlay->event_threshold != 0U) {
            recPlay->event_threshold = 0U;
            osThreadFlagsSet(sdsRecPlayThreadId, 1U << recPlay->index);
          }
        }
      }
    }
  }
  return num;
}

// SDS Player functions

// Open player stream
sdsRecPlayId_t sdsPlayOpen (const char *name, void *buf, uint32_t buf_size, uint32_t io_threshold) {
  sdsRecPlay_t *recPlay = NULL;
  uint32_t      index;

  if ((name != NULL) && (buf != NULL) && (buf_size != 0U) &&
      (buf_size <= SDS_REC_PLAY_BUF_SIZE) && (io_threshold <= buf_size)) {

    recPlay = sdsRecPlayAlloc(&index);
    if (recPlay != NULL) {
      recPlay->index           = index & 0xFFU;
      recPlay->event_threshold = 0U;
      recPlay->event_close     = 0U;
      recPlay->flags           = 0U;
      recPlay->buf_size        = buf_size;
      recPlay->head.timestamp  = 0U;
      recPlay->head.data_size  = 0U;
      recPlay->stream          = sdsOpen(buf, buf_size, io_threshold, 0U);
      recPlay->sdsio           = sdsioOpen(name, sdsioModeRead);

      if (recPlay->stream != NULL) {
        sdsRegisterEvents(recPlay->stream, sdsRecPlayEventCallback, SDS_EVENT_DATA_LOW, (void *)index);
      }
      if ((recPlay->stream == NULL) || (recPlay->sdsio == NULL)) {
        if (recPlay->stream != NULL) {
          sdsClose(recPlay->stream);
          recPlay->stream = NULL;
        }
        if (recPlay->sdsio != NULL) {
          sdsioClose(recPlay->sdsio);
          recPlay->sdsio = NULL;
        }
        sdsRecPlayFree(index);
        recPlay = NULL;
      } else {
        osThreadFlagsSet(sdsRecPlayThreadId, 1U << index);
      }
    }
  }
  return recPlay;
}

// Close player stream
int32_t sdsPlayClose (sdsRecPlayId_t id) {
  sdsRecPlay_t *recPlay = id;
  int32_t       ret  = SDS_REC_PLAY_ERROR;
  uint32_t      event_mask;

  if (recPlay != NULL) {
    event_mask =  1 << recPlay->index;
    recPlay->event_close = 1U;
    osThreadFlagsSet(sdsRecPlayThreadId, event_mask);
    osEventFlagsWait(sdsRecPlayCloseEventFlags, event_mask, osFlagsWaitAll, osWaitForever);

    sdsClose(recPlay->stream);
    sdsioClose(recPlay->sdsio);
    sdsRecPlayFree(recPlay->index);

    ret = SDS_REC_PLAY_OK;
  }
  return ret;
}

// Read data from player stream
uint32_t sdsPlayRead (sdsRecPlayId_t id, uint32_t *timestamp, void *buf, uint32_t buf_size) {
  sdsRecPlay_t *recPlay = id;
  uint32_t      num  = 0U;
  uint32_t      size;

  if ((recPlay != NULL) && (buf != NULL) && (buf_size != 0U)) {
    size = sdsGetCount(recPlay->stream);
    if ((recPlay->head.data_size == 0U) && (size >= HEAD_SIZE)) {
      sdsRead(recPlay->stream, &recPlay->head, HEAD_SIZE);
      size -= HEAD_SIZE;
    }

    if ((recPlay->head.data_size != 0U) && (recPlay->head.data_size <= size)) {
      if (recPlay->head.data_size <= buf_size) {
        num = sdsRead(recPlay->stream, buf, recPlay->head.data_size);
        if (timestamp != NULL) {
          *timestamp = recPlay->head.timestamp;
        }
        recPlay->head.data_size = 0U;
        if (recPlay->event_threshold != 0U) {
          recPlay->event_threshold = 0U;
          osThreadFlagsSet(sdsRecPlayThreadId, 1U << recPlay->index);
        }
      }
    }
  }
  return num;
}

// Get record data size from Player stream
uint32_t sdsPlayGetSize (sdsRecPlayId_t id) {
  sdsRecPlay_t *play = id;
  uint32_t      size = 0U;
  uint32_t      cnt;

  if (play != NULL) {
    if (play->head.data_size == 0U) {
      cnt = sdsGetCount(play->stream);
      if (cnt >= HEAD_SIZE) {
        sdsRead(play->stream, &play->head, HEAD_SIZE);
      }
    }
    size = play->head.data_size;
  }
  return size;
}

// Check if end of stream has been reached
int32_t sdsPlayEndOfStream (sdsRecPlayId_t id) {
  sdsRecPlay_t *recPlay = id;
  int32_t       eos  = 0;

  if (recPlay != NULL) {
    if ((sdsGetCount(recPlay->stream) == 0U) && ((recPlay->flags & SDS_REC_PLAY_FLAG_EOS) != 0U)) {
      eos = 1;
    }
  }
  return eos;
}
