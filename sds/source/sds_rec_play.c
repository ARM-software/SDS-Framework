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
#include "sds_buffer.h"
#include "sdsio.h"
#include "sds_rec_play.h"
#include "sds_rec_play_config.h"

#if SDS_REC_PLAY_MAX_STREAMS > 31
#error "Maximum number of SDS Recorder/Player streams is 31!"
#endif

// Record header
typedef struct {
  uint32_t    timestamp;        // Timestamp in ticks
  uint32_t    data_size;        // Size of a data block in bytes
} recPlayHead_t;
#define HEAD_SIZE sizeof(recPlayHead_t)

// Control block
typedef struct {
         uint8_t        index;              // Index of the RecPlay stream in pRecPlayStreams array
         uint8_t        mode;               // Stream mode (SDS_REC_PLAY_MODE_REC or SDS_REC_PLAY_MODE_PLAY)
         uint8_t        event_threshold;    // Threshold event flag
volatile uint8_t        flags;              // Stream flags: SDS_REC_PLAY_FLAG_ ..
volatile uint32_t       state;              // Stream state: SDS_REC_PLAY_STATE_ ..
volatile uint32_t       lock;               // Lock for atomic operations
         uint32_t       buf_size;           // Size of the buffer used for the stream
         uint32_t       threshold;          // Threshold value
         sdsBufferId_t  sds_buffer;         // SDS Buffer stream handle
         sdsioId_t      sdsio;              // SDS I/O interface handle
         recPlayHead_t  head;               // Header information
} sdsRecPlay_t;

// Allocate memory for the record/playback streams depending on configured maximum number of streams.
static sdsRecPlay_t   RecPlayStreams[SDS_REC_PLAY_MAX_STREAMS] = {0};
static sdsRecPlay_t *pRecPlayStreams[SDS_REC_PLAY_MAX_STREAMS] = {NULL};

// Initialization flag
static uint8_t sdsRecPlayInitialized = 0U;

// Record buffer
static uint8_t sdsRecPlayBuf[SDS_REC_PLAY_BUF_SIZE];

// Event callback
static sdsRecPlayEvent_t sdsRecPlayEvent = NULL;

// Thread Resources
static const osThreadAttr_t sdsRecPlayThreadAttr = {
  "SDS_RecPlay_Thread",
  osThreadDetached,
  NULL, 0, NULL, 0,
  SDS_REC_PLAY_THREAD_PRIORITY,
  0, 0
};
static osThreadId_t sdsRecPlayThreadId;

// Close event flags
static osEventFlagsId_t sdsRecPlayCloseEventFlags;

// Open event flags
static osEventFlagsId_t sdsRecPlayOpenEventFlags;

// Event definitions
#define SDS_REC_PLAY_EVENT_FLAG_MASK ((1UL << SDS_REC_PLAY_MAX_STREAMS) - 1)

// Flags definitions
#define SDS_REC_PLAY_FLAG_HALT          (1U << 0)
#define SDS_REC_PLAY_FLAG_INITIAL_FILL  (1U << 1)
#define SDS_REC_PLAY_FLAG_EOS           (1U << 2)

// Stream mode definitions
#define SDS_REC_PLAY_MODE_REC           (1U << 0)
#define SDS_REC_PLAY_MODE_PLAY          (1U << 1)

// Stream state definitions
#define SDS_REC_PLAY_STATE_INACTIVE     0U // Data stream unused
#define SDS_REC_PLAY_STATE_OPENING      1U // Open function is processed
#define SDS_REC_PLAY_STATE_PLAY         2U // Data stream is in play (read) mode
#define SDS_REC_PLAY_STATE_REC          3U // Data stream is in recording (write) mode
#define SDS_REC_PLAY_STATE_CLOSING      4U // Close function is processed

// Helper functions:

// Atomic Operation: Write 32-bit value to memory if existing value in memory is zero.
//  Return: 1 when new value is written or 0 otherwise.
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

static uint32_t sdsRecPlayLockAcquire (sdsRecPlay_t *rec_play, uint32_t timeout) {
  uint32_t *pLock = (uint32_t *)&rec_play->lock;
  uint32_t  lock  = 0U;
  uint32_t  expirationTick;


  expirationTick = osKernelGetTickCount() + timeout;
  do {
    if (atomic_wr32_if_zero(pLock, 1) != 0U) {
      // Lock acquired.
      lock = 1U;
      break;
    } else {
      osDelay(1);
    }
  } while (osKernelGetTickCount() < expirationTick);

  return lock;
}

static void sdsRecPlayLockRelease (sdsRecPlay_t *rec_play) {
  rec_play->lock = 0U;
}

static sdsRecPlay_t * sdsRecPlayAlloc (uint32_t *index) {
  sdsRecPlay_t   *rec_play = NULL;
  static uint32_t idx = 0U;
  uint32_t        n;

  for (n = 0U; n < SDS_REC_PLAY_MAX_STREAMS; n++) {
    if (atomic_wr32_if_zero((uint32_t *)&pRecPlayStreams[idx], (uint32_t)&RecPlayStreams[idx]) != 0U) {
      rec_play = &RecPlayStreams[idx];
      if (index != NULL) {
        *index = idx;
      }
      break;
    }
    idx++;
    if (idx >= SDS_REC_PLAY_MAX_STREAMS) {
      idx = 0U;
    }
  }
  return rec_play;
}

static void sdsRecPlayFree (uint32_t index) {
  pRecPlayStreams[index] = NULL;
}

// Translate SDSIO error codes to SDS Rec Play error codes.
static int32_t sdsRecPlayTranslateErr (int32_t sdsio_err) {
  int32_t ret;

  switch (sdsio_err) {
    case SDSIO_OK:
      ret = SDS_REC_PLAY_OK;
      break;
    case SDSIO_ERROR:
      ret = SDS_REC_PLAY_ERROR_IO;
      break;
    case SDSIO_ERROR_TIMEOUT:
      ret = SDS_REC_PLAY_ERROR_TIMEOUT;
      break;
    case SDSIO_ERROR_PARAMETER:
      ret = SDS_REC_PLAY_ERROR_PARAMETER;
      break;
    case SDSIO_ERROR_INTERFACE:
    case SDSIO_ERROR_NO_SERVER:
    default:
      ret = SDS_REC_PLAY_ERROR_IO;
      break;
  }
  return ret;
}

// Event callback.
static void sdsRecPlayEventCallback (sdsBufferId_t id, uint32_t event, void *arg) {
  uint32_t      index = (uint32_t)arg;
  sdsRecPlay_t *rec_play;
  (void)id;
  (void)event;

  rec_play = pRecPlayStreams[index];
  if (rec_play != NULL) {
    rec_play->event_threshold = 1U;
  }
}

// Recorder Handler.
static void sdsRecHandler (sdsRecPlay_t *rec_play) {
  uint32_t bytes_remaining, bytes_to_transfer, bytes_transferred, state;
  int32_t  sdsio_ret;

  if ((rec_play->flags & SDS_REC_PLAY_FLAG_HALT) != 0U) {
    // The state of the stream is either closing or inactive.
    // Closing state has been processed by sdsRecPlayThread. Exit the function.
    return;
  }

  state = rec_play->state;

  // Get number of data bytes in the stream buffer.
  bytes_remaining = sdsBufferGetCount(rec_play->sds_buffer);

  while (bytes_remaining != 0U) {
    bytes_to_transfer = bytes_remaining;
    if (bytes_to_transfer > sizeof(sdsRecPlayBuf)) {
      // Adjust number of bytes to transfer to size of intermediate buffer, so that data fits into the buffer.
      bytes_to_transfer = sizeof(sdsRecPlayBuf);
    }

    // Read data from the SDS Stream Buffer to intermediate buffer.
    sdsBufferRead(rec_play->sds_buffer, sdsRecPlayBuf, bytes_to_transfer);

    // Write data from intermediate buffer to SDS I/O Interface.
    sdsio_ret = sdsioWrite(rec_play->sdsio, sdsRecPlayBuf, bytes_to_transfer);
    if (sdsio_ret >= 0) {
      // Number of bytes written to the SDS I/O interface
      bytes_transferred = sdsio_ret;
    } else {
      if (sdsRecPlayEvent != NULL) {
        // Notify the application about I/O error.
        sdsRecPlayEvent(rec_play, SDS_REC_PLAY_EVENT_ERROR_IO);
      }
      break;
    }

    // Update remaining bytes to be transferred.
    if (state != SDS_REC_PLAY_STATE_CLOSING) {
      bytes_remaining -= bytes_transferred;
    } else {
      // State of the stream is closing. Transfer all available data.
      bytes_remaining = sdsBufferGetCount(rec_play->sds_buffer);
    }
  }

  if (state == SDS_REC_PLAY_STATE_CLOSING) {
    // State of the stream is closing.
    // All data has been successfully read from the stream buffer and written to the SDS I/O interface.

    // Set the internal SDS_REC_PLAY_FLAG_HALT flag to mark that sdsRecPlayThread has finished processing the closing state.
    rec_play->flags |= SDS_REC_PLAY_FLAG_HALT;
    // Notify the thread waiting for the event in the sdsRecClose function to finalize the closing of the recorder stream.
    osEventFlagsSet(sdsRecPlayCloseEventFlags, 1U << rec_play->index);
  }
}

// Player Handler.
static void sdsPlayHandler (sdsRecPlay_t *rec_play) {
  uint32_t bytes_remaining, bytes_to_transfer, bytes_transferred;
  int32_t  sdsio_ret;

  if ((rec_play->flags & SDS_REC_PLAY_FLAG_HALT) != 0U) {
    // State of the stream is closing or inactive.
    // Closing state has already been processed by sdsRecPlayThread. Exit the function.
    return;
  }
  if (rec_play->state == SDS_REC_PLAY_STATE_CLOSING) {
    // State of the stream is closing. Thread sdsRecPlayThread has stopped reading data from SDS I/O interface.
    // Set the internal SDS_REC_PLAY_FLAG_HALT flag to mark that sdsRecPlayThread has finished processing the closing state.
    rec_play->flags |= SDS_REC_PLAY_FLAG_HALT;
    // Notify the thread waiting for the event in the sdsPlayClose function to finalize the closing of the player stream.
    osEventFlagsSet(sdsRecPlayCloseEventFlags, 1U << rec_play->index);
    return;
  }

  // Calculate available space in the stream buffer.
  bytes_remaining = rec_play->buf_size - sdsBufferGetCount(rec_play->sds_buffer);
  while (bytes_remaining != 0U) {
    bytes_to_transfer = bytes_remaining;
    if (bytes_to_transfer > sizeof(sdsRecPlayBuf)) {
      // Adjust number of bytes to transfer to size of intermediate buffer, so that data fits into the buffer.
      bytes_to_transfer = sizeof(sdsRecPlayBuf);
    }

    // Read data from the SDS I/O Interface to intermediate buffer.
    sdsio_ret = sdsioRead(rec_play->sdsio, sdsRecPlayBuf, bytes_to_transfer);
    if (sdsio_ret > 0) {
      // Number of bytes read from the SDS I/O interface
      bytes_transferred = sdsio_ret;
    } else if (sdsio_ret == SDSIO_EOS) {
      // End of stream reached.
      bytes_transferred = 0U;
      rec_play->flags |= SDS_REC_PLAY_FLAG_EOS;
    } else {
      // Error occurred during reading from the SDS I/O interface.
      if (sdsRecPlayEvent != NULL) {
        // Notify the application about I/O error.
        sdsRecPlayEvent(rec_play, SDS_REC_PLAY_EVENT_ERROR_IO);
      }
      break;
    }

    if (bytes_transferred != 0U) {
      // Write data from intermediate buffer to the SDS Stream Buffer.
      // Available space in stream buffer has been validated, so write operation is expected to succeed.
      sdsBufferWrite(rec_play->sds_buffer, sdsRecPlayBuf, bytes_transferred);
    }

    // Check if the stream is in the opening state and the processing of the state is not completed.
    if ((rec_play->state == SDS_REC_PLAY_STATE_OPENING) && ((rec_play->flags & SDS_REC_PLAY_FLAG_INITIAL_FILL) == 0U)) {
      // Check if SDS Stream Buffer is filled with data from the SDS I/O interface (at least to threshold or EOS).
      if ((sdsBufferGetCount(rec_play->sds_buffer) >= rec_play->threshold) || ((rec_play->flags & SDS_REC_PLAY_FLAG_EOS) != 0U)) {
        // Stream buffer is filled with data from the SDS I/O interface.
        // Set the internal SDS_REC_PLAY_FLAG_INITIAL_FILL flag to mark that sdsPlayHandler has finished processing the opening state.
        rec_play->flags |= SDS_REC_PLAY_FLAG_INITIAL_FILL;
        // Notify the thread waiting for the event in the sdsPlayOpen function to finalize the opening of the player stream.
        osEventFlagsSet(sdsRecPlayOpenEventFlags, 1U << rec_play->index);
      }
    }

    // Update remaining bytes to transfer.
    bytes_remaining -= bytes_transferred;

    if ((rec_play->flags & SDS_REC_PLAY_FLAG_EOS) != 0U) {
      // End of stream reached. Exiting loop.
      break;
    }
  }
}

// Recorder/Player thread
static __NO_RETURN void sdsRecPlayThread (void *arg) {
  sdsRecPlay_t *rec_play;
  uint32_t      flags, n;

  (void)arg;

  while (1) {
    // Wait for threshold event flags.
    flags = osThreadFlagsWait(SDS_REC_PLAY_EVENT_FLAG_MASK, osFlagsWaitAny, osWaitForever);
    if ((flags & osFlagsError) == 0U) {
      // Process all streams with set flags.
      for (n = 0U; n < SDS_REC_PLAY_MAX_STREAMS; n++) {
        if ((flags & (1U << n)) == 0U) {
          // The flag is not set for this stream. Moving to the next stream.
          continue;
        }
        // Get stream control block.
        rec_play = pRecPlayStreams[n];
        if (rec_play == NULL) {
          // Stream control block is not allocated. Move to next stream.
          continue;
        }
        if (rec_play->state == SDS_REC_PLAY_STATE_INACTIVE) {
          // Stream is inactive. Move to the next stream.
          continue;
        }

        if (rec_play->mode == SDS_REC_PLAY_MODE_REC) {
          // Recorder stream.
          sdsRecHandler(rec_play);
        } else if (rec_play->mode == SDS_REC_PLAY_MODE_PLAY) {
          // Player stream.
          sdsPlayHandler(rec_play);
        }
      }
    }
  }
}

// Initialize recorder and player.
int32_t sdsRecPlayInit (sdsRecPlayEvent_t event_cb) {
  int32_t ret = SDS_REC_PLAY_OK;

  if (sdsRecPlayInitialized != 0U) {
    // The recorder/player are already initialized.
    return SDS_REC_PLAY_OK;
  }

  // Reset an array of pointers to stream control blocks.
  memset(pRecPlayStreams, 0, sizeof(pRecPlayStreams));

  // Initialize SDS I/O interface.
  ret = sdsioInit();
  ret = sdsRecPlayTranslateErr(ret);

  // Create working thread for recorder and player.
  if (ret == SDS_REC_PLAY_OK) {
    sdsRecPlayThreadId = osThreadNew(sdsRecPlayThread, NULL, &sdsRecPlayThreadAttr);
    if (sdsRecPlayThreadId == NULL) {
      ret = SDS_REC_PLAY_ERROR;
    }
  }

  // Create open event flags.
  if (ret == SDS_REC_PLAY_OK) {
    sdsRecPlayOpenEventFlags = osEventFlagsNew(NULL);
    if (sdsRecPlayOpenEventFlags == NULL) {
      ret = SDS_REC_PLAY_ERROR;
    }
  }

  // Create close event flags.
  if (ret == SDS_REC_PLAY_OK) {
    sdsRecPlayCloseEventFlags = osEventFlagsNew(NULL);
    if (sdsRecPlayCloseEventFlags == NULL) {
      ret = SDS_REC_PLAY_ERROR;
    }
  }

  // Set event callback.
  if (ret == SDS_REC_PLAY_OK) {
    // Initialization successful.
    sdsRecPlayEvent = event_cb;
    sdsRecPlayInitialized = 1U;
  } else {
    // Initialization failed:
    // Terminate thread, delete event flags, and uninitialize SDS I/O interface.
    if (sdsRecPlayThreadId != NULL) {
      osThreadTerminate(sdsRecPlayThreadId);
    }
    if (sdsRecPlayOpenEventFlags != NULL) {
      osEventFlagsDelete(sdsRecPlayOpenEventFlags);
    }
    if (sdsRecPlayCloseEventFlags != NULL) {
      osEventFlagsDelete(sdsRecPlayCloseEventFlags);
    }
    sdsioUninit();
  }

  return ret;
}

// Uninitialize recorder and player.
int32_t sdsRecPlayUninit (void) {

  if (sdsRecPlayInitialized == 0U) {
    // Recorder/Player already not initialized.
    return SDS_REC_PLAY_OK;
  }

  // Clear initialization flag.
  sdsRecPlayInitialized = 0U;

  // Terminate thread and delete event flags.
  osThreadTerminate(sdsRecPlayThreadId);
  osEventFlagsDelete(sdsRecPlayOpenEventFlags);
  osEventFlagsDelete(sdsRecPlayCloseEventFlags);

  // Clear event callback.
  sdsRecPlayEvent = NULL;

  // Uninitialize SDS I/O interface.
  sdsioUninit();

  return SDS_REC_PLAY_OK;
}

// SDS Recorder functions:

// Open recorder stream.
sdsRecPlayId_t sdsRecOpen (const char *name, void *buf, uint32_t buf_size) {
  sdsRecPlay_t *rec_play = NULL;
  uint32_t      index;

  if (sdsRecPlayInitialized == 0U) {
    // The recorder/player is not initialized. Exiting the function.
    return NULL;
  }

  // Check if parameters are valid.
  if ((name != NULL) && (buf != NULL) && (buf_size != 0U)) {

    // Atomic allocation of a new control block for the recorder stream.
    rec_play = sdsRecPlayAlloc(&index);

    if (rec_play != NULL) {
      if (sdsRecPlayLockAcquire(rec_play, SDS_REC_PLAY_OPEN_TOUT) != 0U) {

        // Set control block parameters.
        rec_play->state           = SDS_REC_PLAY_STATE_INACTIVE;
        rec_play->index           = index & 0xFFU;
        rec_play->mode            = SDS_REC_PLAY_MODE_REC;
        rec_play->event_threshold = 0U;
        rec_play->flags           = 0U;
        rec_play->buf_size        = buf_size;

        // Set threshold value for the recorder stream.
        if ((buf_size / 3) < SDS_REC_PLAY_IO_TRANSFER_SIZE) {
          // Set threshold to 1/3 of the buffer size.
          rec_play->threshold = buf_size / 3;
        } else {
          // Set threshold to SDS I/O interface efficient transfer size.
          rec_play->threshold = SDS_REC_PLAY_IO_TRANSFER_SIZE;
        }

        // Open sds stream (buffer) and sdsio stream (sds file).
        rec_play->sds_buffer = sdsBufferOpen(buf, buf_size, 0U, rec_play->threshold);
        rec_play->sdsio = sdsioOpen(name, sdsioModeWrite);

        if (rec_play->sds_buffer != NULL) {
          // Register event callback - high threshold.
          sdsBufferRegisterEvents(rec_play->sds_buffer, sdsRecPlayEventCallback, SDS_BUFFER_EVENT_DATA_HIGH, (void *)index);
        }

        // Check if sds stream (buffer) and sdsio stream (sds file) were opened successfully.
        // If not, close streams and free control block.
        if ((rec_play->sds_buffer == NULL) || (rec_play->sdsio == NULL)) {
          if (rec_play->sds_buffer != NULL) {
            sdsBufferClose(rec_play->sds_buffer);
            rec_play->sds_buffer = NULL;
          }
          if (rec_play->sdsio != NULL) {
            sdsioClose(rec_play->sdsio);
            rec_play->sdsio = NULL;
          }

          sdsRecPlayLockRelease(rec_play);
          sdsRecPlayFree(index);
          rec_play = NULL;
        } else {
          // Streams were successfully opened and control block is initialized.
          rec_play->state = SDS_REC_PLAY_STATE_REC;
          sdsRecPlayLockRelease(rec_play);
        }
      } else {
        // Timeout occurred while waiting for lock.
        rec_play = NULL;
      }
    }
  }
  return rec_play;
}

// Close recorder stream.
int32_t sdsRecClose (sdsRecPlayId_t id) {
  sdsRecPlay_t *rec_play = id;
  int32_t       ret      = SDS_REC_PLAY_ERROR;
  int32_t       err      = SDS_REC_PLAY_OK;
  uint32_t      event_mask, flags;

  if (sdsRecPlayInitialized == 0U) {
    // The recorder/player is not initialized. Exiting the function.
    return SDS_REC_PLAY_ERROR;
  }
  if (rec_play == NULL) {
    // Invalid stream. Exit the function.
    return SDS_REC_PLAY_ERROR_PARAMETER;
  }
  if (sdsRecPlayLockAcquire(rec_play, SDS_REC_PLAY_CLOSE_TOUT) == 0U) {
    // Timeout occurred while waiting for lock.
    return SDS_REC_PLAY_ERROR_TIMEOUT;
  }
  if (rec_play->state != SDS_REC_PLAY_STATE_REC) {
    // Stream is not in recording state. Exit the function.
    sdsRecPlayLockRelease(rec_play);
    return SDS_REC_PLAY_ERROR;
  }

  // Set state to closing.
  rec_play->state = SDS_REC_PLAY_STATE_CLOSING;

  // Before recorder stream is closed, sdsRecPlayThread should send all data in SDS Stream Buffer via SDS I/O interface.
  // Notify sdsRecPlayThread to process this stream by setting the corresponding thread flag.
  event_mask = 1 << rec_play->index;
  osThreadFlagsSet(sdsRecPlayThreadId, event_mask);

  // Wait for notification from sdsRecPlayThread that thread has transferred all data from SDS Stream Buffer.
  flags = osEventFlagsWait(sdsRecPlayCloseEventFlags, event_mask, osFlagsWaitAll, SDS_REC_PLAY_CLOSE_TOUT);
  if ((flags & osFlagsError) != 0U) {
    if (flags == osFlagsErrorTimeout) {
      // Timeout occurred.
      err = SDS_REC_PLAY_ERROR_TIMEOUT;
    } else {
      err = SDS_REC_PLAY_ERROR;
    }
  }

  if (err == SDS_REC_PLAY_OK) {
    // Close SDSIO stream.
    err = sdsioClose(rec_play->sdsio);
    err = sdsRecPlayTranslateErr(err);
  }

  if (err == SDS_REC_PLAY_OK) {
    // Close SDS Buffer.
    sdsBufferClose(rec_play->sds_buffer);

    // Reset control block parameters.
    rec_play->sds_buffer = NULL;
    rec_play->sdsio      = NULL;

    // Set state to inactive.
    rec_play->state = SDS_REC_PLAY_STATE_INACTIVE;

    sdsRecPlayFree(rec_play->index);

    ret = SDS_REC_PLAY_OK;
  } else {
    // Close failed. Set state back to recording.
    rec_play->state = SDS_REC_PLAY_STATE_REC;
    ret = err;
  }
  // Release lock.
  sdsRecPlayLockRelease(rec_play);

  return ret;
}

// Write data to recorder stream.
int32_t sdsRecWrite (sdsRecPlayId_t id, uint32_t timestamp, const void *buf, uint32_t buf_size) {
  sdsRecPlay_t *rec_play = id;
  int32_t       ret      = SDS_REC_PLAY_ERROR;
  recPlayHead_t head;

  if (sdsRecPlayInitialized == 0U) {
    // The recorder/player is not initialized. Exiting the function.
    return SDS_REC_PLAY_ERROR;
  }
  if (rec_play == NULL) {
    // Invalid stream. Exit the function.
    return SDS_REC_PLAY_ERROR_PARAMETER;
  }
  if (sdsRecPlayLockAcquire(rec_play, 0) == 0U) {
    // Failed to acquire lock.
    return SDS_REC_PLAY_ERROR;
  }
  if (rec_play->state != SDS_REC_PLAY_STATE_REC) {
    // Stream is not in recording state. Exit the function.
    sdsRecPlayLockRelease(rec_play);
    return SDS_REC_PLAY_ERROR;
  }

  // Verify if parameters are valid.
  if ((buf != NULL) && (buf_size != 0U)) {

    // Check if header + data fits into the buffer.
    if ((buf_size + sizeof(recPlayHead_t)) <= (rec_play->buf_size - sdsBufferGetCount(rec_play->sds_buffer))) {
      // Header: timestamp, data block size.
      head.timestamp = timestamp;
      head.data_size = buf_size;

      // Write header and data block: Buffer size has been validated, so write operations are expected to succeed.
      sdsBufferWrite(rec_play->sds_buffer, &head, sizeof(recPlayHead_t));
      ret = sdsBufferWrite(rec_play->sds_buffer, buf, buf_size);

      // If an SDS threshold event occurred during sdsWrite (amount of data in the SDS Stream Buffer is at or above the threshold),
      // notify the sdsRecPlayThread by setting the corresponding thread flag to process the stream.
      if (rec_play->event_threshold != 0U) {
        rec_play->event_threshold = 0U;
        osThreadFlagsSet(sdsRecPlayThreadId, 1U << rec_play->index);
      }
    } else {
      // Insufficient space in the stream buffer.
      if (sdsRecPlayEvent != NULL) {
        // Notify the application about the error.
        sdsRecPlayEvent(rec_play, SDS_REC_EVENT_ERROR_NO_SPACE);
      }
      ret = SDS_REC_ERROR_NO_SPACE;
    }
  } else {
    ret = SDS_REC_PLAY_ERROR_PARAMETER;
  }

  // Release lock.
  sdsRecPlayLockRelease(rec_play);

  return ret;
}

// SDS Player functions:

// Open player stream.
sdsRecPlayId_t sdsPlayOpen (const char *name, void *buf, uint32_t buf_size) {
  sdsRecPlay_t *rec_play = NULL;
  int32_t       err      = SDS_REC_PLAY_OK;
  uint32_t      index, flags;

  if (sdsRecPlayInitialized == 0U) {
    // The recorder/player is not initialized. Exiting the function.
    return NULL;
  }

  // Check if parameters are valid.
  if ((name != NULL) && (buf != NULL) && (buf_size != 0U)) {

    // Atomic allocation of a new control block for the player stream.
    rec_play = sdsRecPlayAlloc(&index);
    if (rec_play != NULL) {
      if (sdsRecPlayLockAcquire(rec_play, SDS_REC_PLAY_OPEN_TOUT) != 0U) {

        // Set control block parameters.
        rec_play->state           = SDS_REC_PLAY_STATE_INACTIVE;
        rec_play->index           = index & 0xFFU;
        rec_play->mode            = SDS_REC_PLAY_MODE_PLAY;
        rec_play->event_threshold = 0U;
        rec_play->flags           = 0U;
        rec_play->buf_size        = buf_size;
        rec_play->head.timestamp  = 0U;
        rec_play->head.data_size  = 0U;

        // Set threshold value for the recorder stream.
        if ((buf_size / 3) < SDS_REC_PLAY_IO_TRANSFER_SIZE) {
          // Set threshold to 1/3 of the buffer size.
          rec_play->threshold = buf_size - (buf_size / 3);
        } else {
          // Set threshold to SDS I/O interface efficient transfer size.
          rec_play->threshold = buf_size - SDS_REC_PLAY_IO_TRANSFER_SIZE;
        }

        // Open sds stream (buffer) and sdsio stream (sds file).
        rec_play->sds_buffer = sdsBufferOpen(buf, buf_size, rec_play->threshold, 0U);
        rec_play->sdsio = sdsioOpen(name, sdsioModeRead);

        if (rec_play->sds_buffer != NULL) {
          // Register event callback - low threshold.
          sdsBufferRegisterEvents(rec_play->sds_buffer, sdsRecPlayEventCallback, SDS_BUFFER_EVENT_DATA_LOW, (void *)index);
        }

        // Check if sds stream (buffer) and sdsio stream (sds file) were opened successfully.
        if ((rec_play->sds_buffer != NULL) && (rec_play->sdsio != NULL)) {
          // Streams were successfully opened. Set state to opening.
          rec_play->state = SDS_REC_PLAY_STATE_OPENING;
          // Player stream is in the opening state and the sdsRecPlayThread should start
          // reading data from the SDS I/O interface into the SDS Stream Buffer.
          // Notify sdsRecPlayThread to process this stream by setting the corresponding thread flag.
          osThreadFlagsSet(sdsRecPlayThreadId, 1U << index);

          // Wait for notification from sdsRecPlayThread that the stream buffer is filled with the data.
          flags = osEventFlagsWait(sdsRecPlayOpenEventFlags, 1U << index, osFlagsWaitAll, SDS_REC_PLAY_OPEN_TOUT);
          if ((flags & osFlagsError) != 0U) {
            // Timeout or any other error occurred.
            err = SDS_REC_PLAY_ERROR;
          }
        } else {
          err = SDS_REC_PLAY_ERROR;
        }

        if (err == SDS_REC_PLAY_OK) {
          // Streams were successfully opened, control block is initialized and SDS Stream Buffer is filled.
          // Set state to playing.
          rec_play->state = SDS_REC_PLAY_STATE_PLAY;
          sdsRecPlayLockRelease(rec_play);
        } else {
          // Error occurred: Close streams, reset state, free control block and release lock.
          if (rec_play->sds_buffer != NULL) {
            sdsBufferClose(rec_play->sds_buffer);
            rec_play->sds_buffer = NULL;
          }
          if (rec_play->sdsio != NULL) {
            sdsioClose(rec_play->sdsio);
            rec_play->sdsio = NULL;
          }
          rec_play->state = SDS_REC_PLAY_STATE_INACTIVE;

          sdsRecPlayLockRelease(rec_play);

          sdsRecPlayFree(index);
          rec_play = NULL;
        }
      } else {
        // Timeout occurred while waiting for lock.
        rec_play = NULL;
      }
    }
  }
  return rec_play;
}

// Close player stream.
int32_t sdsPlayClose (sdsRecPlayId_t id) {
  sdsRecPlay_t *rec_play = id;
  int32_t       ret      = SDS_REC_PLAY_ERROR;
  int32_t       err      = SDS_REC_PLAY_OK;
  uint32_t      event_mask, flags;

  if (sdsRecPlayInitialized == 0U) {
    // The recorder/player is not initialized. Exiting the function.
    return SDS_REC_PLAY_ERROR;
  }
  if (rec_play == NULL) {
    // Invalid stream. Exit the function.
    return SDS_REC_PLAY_ERROR_PARAMETER;
  }
  if (sdsRecPlayLockAcquire(rec_play, SDS_REC_PLAY_CLOSE_TOUT) == 0U) {
    // Timeout occurred while waiting for lock.
    return SDS_REC_PLAY_ERROR_TIMEOUT;
  }
  if (rec_play->state != SDS_REC_PLAY_STATE_PLAY) {
    // Stream is not in playing state. Exit the function.
    sdsRecPlayLockRelease(rec_play);
    return SDS_REC_PLAY_ERROR;
  }

  // Set state to closing.
  rec_play->state = SDS_REC_PLAY_STATE_CLOSING;

  // Before player stream is closed, sdsRecPlayThread should stop reading data from SDS I/O interface.
  // Notify sdsRecPlayThread to process this stream by setting the corresponding thread flag.
  event_mask = 1 << rec_play->index;
  osThreadFlagsSet(sdsRecPlayThreadId, event_mask);

  // Wait for notification from sdsRecPlayThread that thread has stopped reading data.
  flags = osEventFlagsWait(sdsRecPlayCloseEventFlags, event_mask, osFlagsWaitAll, SDS_REC_PLAY_CLOSE_TOUT);
  if ((flags & osFlagsError) != 0U) {
    if (flags == osFlagsErrorTimeout) {
      // Timeout occurred.
      err = SDS_REC_PLAY_ERROR_TIMEOUT;
    } else {
      err = SDS_REC_PLAY_ERROR;
    }
  }

  if (err == SDS_REC_PLAY_OK) {
    // Close SDSIO stream.
    err = sdsioClose(rec_play->sdsio);
    err = sdsRecPlayTranslateErr(err);
  }

  if (err == SDS_REC_PLAY_OK) {
    // Close SDS Buffer.
    sdsBufferClose(rec_play->sds_buffer);

    // Reset control block parameters.
    rec_play->sds_buffer = NULL;
    rec_play->sdsio      = NULL;

    // Set state to inactive.
    rec_play->state = SDS_REC_PLAY_STATE_INACTIVE;

    sdsRecPlayLockRelease(rec_play);
    sdsRecPlayFree(rec_play->index);

    ret = SDS_REC_PLAY_OK;
  } else {
    // Close failed. Set state back to playing.
    rec_play->state = SDS_REC_PLAY_STATE_PLAY;
    ret = err;
    sdsRecPlayLockRelease(rec_play);
  }

  return ret;
}

// Read data from player stream.
int32_t sdsPlayRead (sdsRecPlayId_t id, uint32_t *timestamp, void *buf, uint32_t buf_size) {
  sdsRecPlay_t *rec_play = id;
  int32_t       ret      = SDS_REC_PLAY_ERROR;
  int32_t       eos      = 0;
  int32_t       size;

  if (sdsRecPlayInitialized == 0U) {
    // The recorder/player is not initialized. Exiting the function.
    return SDS_REC_PLAY_ERROR;
  }
  if (rec_play == NULL) {
    // Invalid stream. Exit the function.
    return SDS_REC_PLAY_ERROR_PARAMETER;
  }
  if (sdsRecPlayLockAcquire(rec_play, 0) == 0U) {
    // Failed to acquire lock.
    return SDS_REC_PLAY_ERROR;
  }
  if (rec_play->state != SDS_REC_PLAY_STATE_PLAY) {
    // Stream is not in playing state. Exit the function.
    sdsRecPlayLockRelease(rec_play);
    return SDS_REC_PLAY_ERROR;
  }

  // Verify if parameters are valid.
  if ((buf != NULL) && (buf_size != 0U)) {
    // Save eos flag.
    eos = rec_play->flags & SDS_REC_PLAY_FLAG_EOS;
    // Get size of available data block.
    size = sdsBufferGetCount(rec_play->sds_buffer);

    // Check if header was already read:
    // If not (head.data_size == 0) and there is enough data available, read new header.
    if ((rec_play->head.data_size == 0U) && (size >= HEAD_SIZE)) {
      sdsBufferRead(rec_play->sds_buffer, &rec_play->head, HEAD_SIZE);
      size -= HEAD_SIZE;
    }

    // Check if Header is valid and data block size in header is valid.
    if (rec_play->head.data_size != 0U) {
      // Check if whole data block fits into the provided buffer.
      if (rec_play->head.data_size > buf_size) {
        // Provided buffer is too small to read the data block.
        ret = SDS_REC_PLAY_ERROR_PARAMETER;
      } else {
        // Check if whole data block is available in the SDS Stream Buffer.
        if (rec_play->head.data_size > size) {
          // Whole data block is not available in the SDS Stream Buffer.
          ret = SDS_PLAY_ERROR_NO_DATA;
        } else {
          // Read data block from SDS Stream Buffer:
          // Buffer size has been validated, so read operation is expected to succeed.
          ret = sdsBufferRead(rec_play->sds_buffer, buf, rec_play->head.data_size);

          // Get timestamp from the header.
          if (timestamp != NULL) {
            *timestamp = rec_play->head.timestamp;
          }

          // Whole data block has been read from the SDS Stream Buffer.
          // Clear the header information to enable reading of the next data block.
          rec_play->head.data_size = 0U;

          // If an SDS threshold event occurred during sdsRead (amount of data in the SDS Stream Buffer is at or below the threshold),
          // notify the sdsRecPlayThread by setting the corresponding thread flag to process the stream.
          if (rec_play->event_threshold != 0U) {
            rec_play->event_threshold = 0U;
            osThreadFlagsSet(sdsRecPlayThreadId, 1U << rec_play->index);
          }
        }
      }
    } else {
      // Header is not valid.
      // Check for End Of Stream.
      if ((size == 0) && (eos != 0U)) {
        ret = SDS_PLAY_EOS;
      } else {
        // Whole header is not available in the SDS Stream Buffer.
        ret = SDS_PLAY_ERROR_NO_DATA;
      }
    }
  } else {
    ret = SDS_REC_PLAY_ERROR_PARAMETER;
  }

  if ((ret == SDS_PLAY_ERROR_NO_DATA) && (sdsRecPlayEvent != NULL)) {
    // Notify the application about the error.
    sdsRecPlayEvent(rec_play, SDS_PLAY_EVENT_ERROR_NO_DATA);
  }

  // Release Lock.
  sdsRecPlayLockRelease(rec_play);

  return ret;
}

// Get data block size from Player stream
int32_t sdsPlayGetSize (sdsRecPlayId_t id) {
  sdsRecPlay_t *rec_play = id;
  int32_t       ret      = SDS_REC_PLAY_ERROR;
  int32_t       eos      = 0;
  int32_t       size;

  if (sdsRecPlayInitialized == 0U) {
    // The recorder/player is not initialized. Exiting the function.
    return SDS_REC_PLAY_ERROR;
  }
  if (rec_play == NULL) {
    // Invalid stream. Exit the function.
    return SDS_REC_PLAY_ERROR_PARAMETER;
  }
  if (sdsRecPlayLockAcquire(rec_play, 0) == 0U) {
    // Failed to acquire lock.
    return SDS_REC_PLAY_ERROR;
  }
  if (rec_play->state != SDS_REC_PLAY_STATE_PLAY) {
    // Stream is not in playing state. Exit the function.
    sdsRecPlayLockRelease(rec_play);
    return SDS_REC_PLAY_ERROR;
  }

  // Save eos flag.
  eos = rec_play->flags & SDS_REC_PLAY_FLAG_EOS;
  // Get size of available data block.
  size = sdsBufferGetCount(rec_play->sds_buffer);

  // Check if header was already read:
  // If not (head.data_size == 0) and there is enough data available, read new header.
  if ((rec_play->head.data_size == 0U) && (size >= HEAD_SIZE)) {
    sdsBufferRead(rec_play->sds_buffer, &rec_play->head, HEAD_SIZE);
    size -= HEAD_SIZE;
  }

  // Check if Header is valid and data block size in header is valid.
  if (rec_play->head.data_size != 0U) {
    if (rec_play->head.data_size > size) {
      // Whole data block is not available in the SDS Stream Buffer.
      ret = SDS_PLAY_ERROR_NO_DATA;
    } else {
      // Return size of the data block.
      ret = rec_play->head.data_size;
    }
  } else {
    // Header is not valid.
    // Check for End Of Stream.
    if ((size == 0) && (eos != 0U)) {
      ret = SDS_PLAY_EOS;
    } else {
      // Whole header is not available in the SDS Stream Buffer.
      ret = SDS_PLAY_ERROR_NO_DATA;
    }
  }

  // Release Lock.
  sdsRecPlayLockRelease(rec_play);
  return ret;
}
