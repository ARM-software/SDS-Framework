/*
 * Copyright (c) 2023-2024 Arm Limited. All rights reserved.
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

// SDS Player

#include <stdatomic.h>
#include <string.h>

#include "cmsis_compiler.h"
#include "cmsis_os2.h"
#include "sds.h"
#include "sdsio.h"
#include "sds_play.h"
#include "sds_play_config.h"

#if SDS_PLAY_MAX_STREAMS > 31
#error "Maximum number of SDS Player streams is 31!"
#endif

// Player header
typedef struct {
  uint32_t    timestamp;        // Timestamp in ticks
  uint32_t    data_size;        // Data size in bytes
} PlayHead_t;
#define HEAD_SIZE sizeof(PlayHead_t)

// Control block
typedef struct {
           uint8_t     index;
           uint8_t     event_threshold;
           uint8_t     event_close;
           uint8_t     flags;
           uint32_t    buf_size;
           sdsId_t     stream;
           sdsioId_t   sdsio;
           PlayHead_t  head_out;
           uint32_t    record_data_cnt;
} sdsPlay_t;

static sdsPlay_t   PlayStreams[SDS_PLAY_MAX_STREAMS] = {0};
static sdsPlay_t *pPlayStreams[SDS_PLAY_MAX_STREAMS] = {NULL};

// Player buffer
static uint8_t PlayBuf[SDS_PLAY_BUF_SIZE];

// Event callback
static sdsPlayEvent_t sdsPlayEvent = NULL;

// Thread Id
static osThreadId_t sdsPlayThreadId;

// Close event flags
static osEventFlagsId_t sdsPlayCloseEventFlags;

// Event definitions
#define SDS_PLAY_EVENT_FLAG_MASK    ((1UL << SDS_PLAY_MAX_STREAMS) - 1)

// Flag definitions
#define SDS_PLAY_FLAG_CLOSE         (1U << 0)
#define SDS_PLAY_FLAG_EOS           (1U << 1)

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

static sdsPlay_t * sdsPlayAlloc (uint32_t *index) {
  sdsPlay_t *play = NULL;
  uint32_t   n;

  for (n = 0U; n < SDS_PLAY_MAX_STREAMS; n++) {
    if (atomic_wr32_if_zero((uint32_t *)&pPlayStreams[n], (uint32_t)&PlayStreams[n]) != 0U) {
      play = &PlayStreams[n];
      if (index != NULL) {
        *index = n;
      }
      break;
    }
  }
  return play;
}

static void sdsPlayFree (uint32_t index) {
  pPlayStreams[index] = NULL;
}

// Event callback
static void sdsPlayEventCallback (sdsId_t id, uint32_t event, void *arg) {
  uint32_t   index = (uint32_t)arg;
  sdsPlay_t *play;
  (void)id;
  (void)event;

  play = pPlayStreams[index];
  if (play != NULL) {
    play->event_threshold = 1U;
  }
}

// Player thread
static __NO_RETURN void sdsPlayThread (void *arg) {
  sdsPlay_t *play;
  uint32_t  flags, cnt, num, n;
  (void)arg;

  while (1) {
    flags = osThreadFlagsWait(SDS_PLAY_EVENT_FLAG_MASK, osFlagsWaitAny, osWaitForever);
    if ((flags & osFlagsError) == 0U) {
      for (n = 0U; n < SDS_PLAY_MAX_STREAMS; n++) {
        if ((flags & (1U << n)) == 0U) {
          continue;
        }
        play = pPlayStreams[n];
        if (play == NULL) {
          continue;
        }
        if ((play->flags & SDS_PLAY_FLAG_CLOSE) != 0U) {
          continue;
        }
        if (play->event_close != 0U) {
          play->flags |= SDS_PLAY_FLAG_CLOSE;
          osEventFlagsSet(sdsPlayCloseEventFlags, 1U << play->index);
          continue;
        }

        while (1) {
          cnt = play->buf_size - sdsGetCount(play->stream);
          if (cnt == 0U) {
            break;
          }
          if (cnt > sizeof(PlayBuf)) {
            cnt = sizeof(PlayBuf);
          }
          num = sdsioRead(play->sdsio, PlayBuf, cnt);
          if (num != 0U) {
            sdsWrite(play->stream, PlayBuf, num);
          }
          if (num != cnt) {
            if (sdsioEndOfStream(play->sdsio) != 0) {
              play->flags |= SDS_PLAY_FLAG_EOS;
            } else {
              if (sdsPlayEvent != NULL) {
                sdsPlayEvent(play, SDS_PLAY_EVENT_IO_ERROR);
              }
            }
            break;
          }
        }
      }
    }
  }
}

// SDS Player functions

// Initialize player
int32_t sdsPlayInit (sdsPlayEvent_t event_cb) {
  int32_t ret = SDS_PLAY_ERROR;

  (void)event_cb;

  memset(pPlayStreams, 0, sizeof(pPlayStreams));

  if (sdsioInit() == SDSIO_OK) {
    sdsPlayThreadId = osThreadNew(sdsPlayThread, NULL, NULL);
    if (sdsPlayThreadId != NULL)  {
      sdsPlayCloseEventFlags = osEventFlagsNew(NULL);
      if (sdsPlayCloseEventFlags != NULL) {
        sdsPlayEvent = event_cb;
        ret = SDS_PLAY_OK;
      }
    }
  }
  return ret;
}

// Uninitialize player
int32_t sdsPlayUninit (void) {
  int32_t ret = SDS_PLAY_ERROR;

  osThreadTerminate(sdsPlayThreadId);
  osEventFlagsDelete(sdsPlayCloseEventFlags);
  sdsPlayEvent = NULL;
  sdsioUninit();

  return ret;
}

// Open player stream
sdsPlayId_t sdsPlayOpen (const char *name, void *buf, uint32_t buf_size, uint32_t io_threshold) {
  sdsPlay_t *play = NULL;
  uint32_t   index;

  if ((name != NULL) && (buf != NULL) && (buf_size != 0U) &&
      (buf_size <= SDS_PLAY_BUF_SIZE) && (io_threshold <= buf_size)) {

    play = sdsPlayAlloc(&index);
    if (play != NULL) {
      play->index              = index & 0xFFU;
      play->event_threshold    = 0U;
      play->event_close        = 0U;
      play->flags              = 0U;
      play->buf_size           = buf_size;
      play->head_out.timestamp = 0U;
      play->head_out.data_size = 0U;
      play->record_data_cnt    = 0U;
      play->stream             = sdsOpen(buf, buf_size, io_threshold, 0U);
      play->sdsio              = sdsioOpen(name, sdsioModeRead);

      if (play->stream != NULL) {
        sdsRegisterEvents(play->stream, sdsPlayEventCallback, SDS_EVENT_DATA_LOW, (void *)index);
      }
      if ((play->stream == NULL) || (play->sdsio == NULL)) {
        if (play->stream != NULL) {
          sdsClose(play->stream);
          play->stream = NULL;
        }
        if (play->sdsio != NULL) {
          sdsioClose(play->sdsio);
          play->sdsio = NULL;
        }
        sdsPlayFree(index);
        play = NULL;
      } else {
        osThreadFlagsSet(sdsPlayThreadId, 1U << index);
      }
    }
  }
  return play;
}

// Close player stream
int32_t sdsPlayClose (sdsPlayId_t id) {
  sdsPlay_t *play = id;
  int32_t    ret  = SDS_PLAY_ERROR;
  uint32_t   event_mask;

  if (play != NULL) {
    event_mask =  1 << play->index;
    play->event_close = 1U;
    osThreadFlagsSet(sdsPlayThreadId, event_mask);
    osEventFlagsWait(sdsPlayCloseEventFlags, event_mask, osFlagsWaitAll, osWaitForever);

    sdsClose(play->stream);
    sdsioClose(play->sdsio);
    sdsPlayFree(play->index);

    ret = SDS_PLAY_OK;
  }
  return ret;
}

// Read data from player stream
uint32_t sdsPlayRead (sdsPlayId_t id, uint32_t *timestamp, void *buf, uint32_t buf_size) {
  sdsPlay_t *play = id;
  uint32_t   num  = 0U;
  uint32_t   size;

  if ((play != NULL) && (buf != NULL) && (buf_size != 0U)) {
    size = sdsGetCount(play->stream);
    if ((play->head_out.data_size == 0U) && (size >= HEAD_SIZE)) {
      sdsRead(play->stream, &play->head_out, HEAD_SIZE);
      size -= HEAD_SIZE;
    }

    if ((play->head_out.data_size != 0U) && (play->head_out.data_size <= size)) {
      if (play->head_out.data_size <= buf_size) {
        num = sdsRead(play->stream, buf, play->head_out.data_size);
        if (timestamp != NULL) {
          *timestamp = play->head_out.timestamp;
        }
        play->head_out.data_size = 0U;
        if (play->event_threshold != 0U) {
          play->event_threshold = 0U;
          osThreadFlagsSet(sdsPlayThreadId, 1U << play->index);
        }
      }
    }
  }
  return num;
}

// Get record data size from Player stream
uint32_t sdsPlayGetSize (sdsPlayId_t id) {
  sdsPlay_t *play = id;
  uint32_t   size = 0U;
  uint32_t   cnt;

  if (play != NULL) {
    if (play->head_out.data_size == 0U) {
      cnt = sdsGetCount(play->stream);
      if (cnt >= HEAD_SIZE) {
        sdsRead(play->stream, &play->head_out, HEAD_SIZE);
      }
    }
    size = play->head_out.data_size;
  }
  return size;
}

// Check if end of stream has been reached
int32_t sdsPlayEndOfStream (sdsPlayId_t id) {
  sdsPlay_t *play = id;
  int32_t    eos  = 0;

  if (play != NULL) {
    if ((sdsGetCount(play->stream) == 0U) && ((play->flags & SDS_PLAY_FLAG_EOS) != 0U)) {
      eos = 1;
    }
  }
  return eos;
}
