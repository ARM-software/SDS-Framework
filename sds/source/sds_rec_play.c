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
  uint32_t    data_size;        // Data size in bytes
} recPlayHead_t;
#define HEAD_SIZE sizeof(recPlayHead_t)

// Stream state
typedef struct {
  uint8_t     opening;          // Opening state flag
  uint8_t     opened;           // Opened state flag
  uint8_t     closing;          // Closing state flag
  uint8_t     rw_active;        // Read/Write active state flag
} recPlayState_t;

// Control block
typedef struct {
         uint8_t        index;              // Index of the RecPlay stream in pRecPlayStreams array
         uint8_t        stream_type;        // Type of the stream (SDS_REC_PLAY_TYPE_REC or SDS_REC_PLAY_TYPE_PLAY)
         uint8_t        event_threshold;    // Threshold event flag
volatile uint8_t        flags;              // Stream flags: SDS_REC_PLAY_FLAG_ ..
         uint32_t       buf_size;           // Size of the buffer used for the stream
         uint32_t       threshold;          // Threshold value
volatile recPlayState_t state;              // State of the stream
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

// Thread Id
static osThreadId_t sdsRecPlayThreadId;

// Close event flags
static osEventFlagsId_t sdsRecPlayCloseEventFlags;

// Open event flags
static osEventFlagsId_t sdsRecPlayOpenEventFlags;

// Event definitions
#define SDS_REC_PLAY_EVENT_FLAG_MASK ((1UL << SDS_REC_PLAY_MAX_STREAMS) - 1)

// Stream types
#define SDS_REC_PLAY_TYPE_REC         (1U << 0)
#define SDS_REC_PLAY_TYPE_PLAY        (1U << 1)

// Flag definitions
#define SDS_REC_PLAY_FLAG_OPEN        (1U << 0)
#define SDS_REC_PLAY_FLAG_CLOSE       (1U << 1)
#define SDS_REC_PLAY_FLAG_EOS         (1U << 2)

// Helper functions:

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
  sdsRecPlay_t *rec_play = NULL;
  uint32_t      n;

  for (n = 0U; n < SDS_REC_PLAY_MAX_STREAMS; n++) {
    if (atomic_wr32_if_zero((uint32_t *)&pRecPlayStreams[n], (uint32_t)&RecPlayStreams[n]) != 0U) {
      rec_play = &RecPlayStreams[n];
      if (index != NULL) {
        *index = n;
      }
      break;
    }
  }
  return rec_play;
}

static void sdsRecPlayFree (uint32_t index) {
  pRecPlayStreams[index] = NULL;
}

// Event callback
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

// Recorder Handler
static void sdsRecHandler (sdsRecPlay_t *rec_play) {
  uint32_t bytes_remaining, bytes_to_transfer, bytes_transferred;

  if ((rec_play->flags & SDS_REC_PLAY_FLAG_CLOSE) != 0U) {
    // Stream is in closing state. Exit the function.
    return;
  }

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

    // Write data from intermediate buffer to SDS I/O Interface
    bytes_transferred = sdsioWrite(rec_play->sdsio, sdsRecPlayBuf, bytes_to_transfer);
    if (bytes_transferred != bytes_to_transfer) {
      if (sdsRecPlayEvent != NULL) {
        // Notify the application about I/O error.
        sdsRecPlayEvent(rec_play, SDS_REC_PLAY_EVENT_IO_ERROR);
      }
      break;
    }

    // Update remaining bytes to be transferred
    if (rec_play->state.closing == 0U) {
      bytes_remaining -= bytes_transferred;
    } else {
      // State of the stream is closing. Transfer all available data
      bytes_remaining = sdsBufferGetCount(rec_play->sds_buffer);
    }
  }

  if (rec_play->state.closing != 0U) {
    // State of the stream is closing:
    //   All data has been successfully read from the stream buffer and written to the SDS I/O interface.
    //   Set the internal close flag and notify the thread waiting for the event in the sdsRecClose
    //   function to finalize the closing of the recorder stream.
    rec_play->flags |= SDS_REC_PLAY_FLAG_CLOSE;
    osEventFlagsSet(sdsRecPlayCloseEventFlags, 1U << rec_play->index);
  }
}

// Player Handler
static void sdsPlayHandler (sdsRecPlay_t *rec_play) {
  uint32_t bytes_remaining, bytes_to_transfer, bytes_transferred;

  if ((rec_play->flags & SDS_REC_PLAY_FLAG_CLOSE) != 0U) {
    // Stream is in closing state. Exit the function.
    return;
  }
  if (rec_play->state.closing != 0U) {
    // State of the stream is closing:
    //   Thread sdsRecPlayThread has stopped reading data from SDS I/O interface.
    //   Set the internal close flag and notify the thread waiting for the event in the sdsPlayClose
    //   function to finalize the closing of the player stream.
    rec_play->flags |= SDS_REC_PLAY_FLAG_CLOSE;
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
    bytes_transferred = sdsioRead(rec_play->sdsio, sdsRecPlayBuf, bytes_to_transfer);
    if (bytes_transferred != bytes_to_transfer) {
      // Requested number of bytes was not read from the SDS I/O interface.
      // Check if end of stream has been reached.
      if (sdsioEndOfStream(rec_play->sdsio) != 0) {
        rec_play->flags |= SDS_REC_PLAY_FLAG_EOS;
      } else {
        // It is not end of stream, but an error occurred during reading from the SDS I/O interface.
        if (sdsRecPlayEvent != NULL) {
          // Notify the application about I/O error.
          sdsRecPlayEvent(rec_play, SDS_REC_PLAY_EVENT_IO_ERROR);
          break;
        }
      }
    }

    if (bytes_transferred != 0U) {
      // Write data from intermediate buffer to the SDS Stream Buffer.
      // Available space in stream buffer has been validated, so write operation is expected to succeed.
      sdsBufferWrite(rec_play->sds_buffer, sdsRecPlayBuf, bytes_transferred);
    }

    // Verify if the stream is in the opening state and ensure that the SDS Stream Buffer
    // is initially populated with data from the SDS I/O interface.
    if ((rec_play->flags & SDS_REC_PLAY_FLAG_OPEN) == 0U) {
      if (rec_play->state.opening != 0U) {
        // State of the stream is opening:
        //    Check if SDS Stream Buffer is filled with data from the SDS I/O interface (at least to threshold or EOS).
        if ((sdsBufferGetCount(rec_play->sds_buffer) >= rec_play->threshold) || ((rec_play->flags & SDS_REC_PLAY_FLAG_EOS) != 0U)) {
          // Stream buffer is filled with data from the SDS I/O interface.
          // Set the open flag and notify the thread waiting for the event in the sdsPlayOpen
          // function to finalize the opening of the player stream.
          rec_play->flags |= SDS_REC_PLAY_FLAG_OPEN;
          osEventFlagsSet(sdsRecPlayOpenEventFlags, 1U << rec_play->index);
        }
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
          // Flag is not set for this stream. Move to next stream.
          continue;
        }
        // Get stream control block.
        rec_play = pRecPlayStreams[n];
        if (rec_play == NULL) {
          // Stream control block is not allocated. Move to next stream.
          continue;
        }
        if ((rec_play->state.opened == 0U) && ((rec_play->state.opening == 0U))) {
          // Stream is neither opened nor in the process of opening. Move to the next stream.
          continue;
        }

        if (rec_play->stream_type == SDS_REC_PLAY_TYPE_REC) {
          // Recorder stream.
          sdsRecHandler(rec_play);
        } else if (rec_play->stream_type == SDS_REC_PLAY_TYPE_PLAY) {
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

  // Reset an array of pointers to stream control blocks.
  memset(pRecPlayStreams, 0, sizeof(pRecPlayStreams));

  // Initialize SDS I/O interface.
  if (sdsioInit() != SDSIO_OK) {
    ret = SDS_REC_PLAY_ERROR;
  }

  // Create working thread for recorder and player.
  if (ret == SDS_REC_PLAY_OK) {
    sdsRecPlayThreadId = osThreadNew(sdsRecPlayThread, NULL, NULL);
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
sdsRecPlayId_t sdsRecOpen (const char *name, void *buf, uint32_t buf_size, uint32_t io_threshold) {
  sdsRecPlay_t *rec_play = NULL;
  uint32_t      index;

  if (sdsRecPlayInitialized == 0U) {
    // Recorder/Player is not initialized. Exit the function.
    return NULL;
  }

  // Check if parameters are valid.
  if ((name != NULL) && (buf != NULL) &&
      (buf_size != 0U) && (io_threshold <= buf_size)) {

    // Atomic allocation of a new control block for the recorder stream.
    rec_play = sdsRecPlayAlloc(&index);
    if (rec_play != NULL) {

      // Set control block parameters.
      rec_play->index           = index & 0xFFU;
      rec_play->stream_type     = SDS_REC_PLAY_TYPE_REC;
      rec_play->event_threshold = 0U;
      rec_play->flags           = 0U;
      rec_play->buf_size        = buf_size;
      rec_play->threshold       = io_threshold;

      // Set states.
      rec_play->state.opened    = 0U;
      rec_play->state.closing   = 0U;
      rec_play->state.rw_active = 0U;
      rec_play->state.opening   = 0U; // Opening state is not used for recorder.

      // Open sds stream (buffer) and sdsio stream (sds file).
      rec_play->sds_buffer = sdsBufferOpen(buf, buf_size, 0U, io_threshold);
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
        sdsRecPlayFree(index);
        rec_play = NULL;
      } else {
        // Streams were successfully opened and control block is initialized.
        rec_play->state.opened = 1U;
      }
    }
  }
  return rec_play;
}

// Close recorder stream.
int32_t sdsRecClose (sdsRecPlayId_t id) {
  sdsRecPlay_t *rec_play = id;
  int32_t       ret      = SDS_REC_PLAY_ERROR;
  uint32_t      event_mask, flags;

  if (sdsRecPlayInitialized == 0U) {
    // Recorder/Player is not initialized. Exit the function.
    return SDS_REC_PLAY_ERROR;
  }
  if (rec_play == NULL) {
    // Invalid stream. Exit the function.
    return SDS_REC_PLAY_ERROR;
  }
  if (rec_play->state.opened == 0U) {
    // Stream is not opened. Exit the function.
    return SDS_REC_PLAY_ERROR;
  }

  event_mask = 1 << rec_play->index;

  // Change state to closing.
  rec_play->state.closing = 1U;

  // Wait until writing to SDS Stream Buffer is completed.
  // Writing to SDS Stream Buffer (RAM) is expected to be fast, so this loop should not take long.
  while (rec_play->state.rw_active != 0U);

  // Before recorder stream is closed sdsRecPlayThread should send all data in SDS Stream Buffer via SDS I/O interface.
  // Notify sdsRecPlayThread to process this stream by setting the corresponding thread flag.
  osThreadFlagsSet(sdsRecPlayThreadId, event_mask);

  // Wait for notification from sdsRecPlayThread that thread has transferred all data from SDS Stream Buffer.
  flags = osEventFlagsWait(sdsRecPlayCloseEventFlags, event_mask, osFlagsWaitAll, SDS_REC_PLAY_CLOSE_TOUT);
  if ((flags & osFlagsError) != 0U) {
    if (flags == osFlagsErrorTimeout) {
      // Timeout occurred.
      ret = SDS_REC_PLAY_ERROR_TIMEOUT;
    }
  } else {
    // Close streams and free control block.
    sdsBufferClose(rec_play->sds_buffer);
    sdsioClose(rec_play->sdsio);
    sdsRecPlayFree(rec_play->index);

    // Update states: Stream is closed.
    rec_play->state.opened  = 0U;
    rec_play->state.closing = 0U;

    ret = SDS_REC_PLAY_OK;
  }

  return ret;
}

// Write data to recorder stream.
uint32_t sdsRecWrite (sdsRecPlayId_t id, uint32_t timestamp, const void *buf, uint32_t buf_size) {
  sdsRecPlay_t *rec_play = id;
  uint32_t      num      = 0U;
  recPlayHead_t head;

  if (sdsRecPlayInitialized == 0U) {
    // Recorder/Player is not initialized. Exit the function.
    return 0U;
  }
  if (rec_play == NULL) {
    // Invalid stream. Exit the function.
    return 0U;
  }
  if ((rec_play->state.opened == 0U) || (rec_play->state.closing != 0U)) {
    // Stream is closing or already closed. Exit the function.
    return 0U;
  }

  // Set read/write active state.
  rec_play->state.rw_active = 1U;

  // Ensure the stream state has not transitioned to closing.
  if (rec_play->state.closing == 0U) {
    // Verify if parameters are valid.
    if ((buf != NULL) && (buf_size != 0U)) {

      // Check if header + data fits into the buffer.
      if ((buf_size + sizeof(recPlayHead_t)) <= (rec_play->buf_size - sdsBufferGetCount(rec_play->sds_buffer))) {
        // Header: timestamp, data size.
        head.timestamp = timestamp;
        head.data_size = buf_size;

        // Write header and data: Buffer size has been validated, so write operations are expected to succeed.
        sdsBufferWrite(rec_play->sds_buffer, &head, sizeof(recPlayHead_t));
        num = sdsBufferWrite(rec_play->sds_buffer, buf, buf_size);

        // If an SDS threshold event occurred during sdsWrite (amount of data in the SDS Stream Buffer is at or above the threshold),
        // notify the sdsRecPlayThread by setting the corresponding thread flag to process the stream.
        if (rec_play->event_threshold != 0U) {
          rec_play->event_threshold = 0U;
          osThreadFlagsSet(sdsRecPlayThreadId, 1U << rec_play->index);
        }
      }
    }
  }
  // Clear read/write active state.
  rec_play->state.rw_active = 0U;

  return num;
}

// SDS Player functions:

// Open player stream.
sdsRecPlayId_t sdsPlayOpen (const char *name, void *buf, uint32_t buf_size, uint32_t io_threshold) {
  sdsRecPlay_t *rec_play = NULL;
  uint8_t       err      = 0U;
  uint32_t      index, flags;

  if (sdsRecPlayInitialized == 0U) {
    // Recorder/Player is not initialized. Exit the function.
    return NULL;
  }

  // Check if parameters are valid.
  if ((name != NULL) && (buf != NULL) &&
      (buf_size != 0U) && (io_threshold <= buf_size)) {

    // Atomic allocation of a new control block for the player stream.
    rec_play = sdsRecPlayAlloc(&index);
    if (rec_play != NULL) {

      // Set control block parameters.
      rec_play->index           = index & 0xFFU;
      rec_play->stream_type     = SDS_REC_PLAY_TYPE_PLAY;
      rec_play->event_threshold = 0U;
      rec_play->flags           = 0U;
      rec_play->buf_size        = buf_size;
      rec_play->threshold       = io_threshold;
      rec_play->head.timestamp  = 0U;
      rec_play->head.data_size  = 0U;

      // Set states.
      rec_play->state.opened    = 0U;
      rec_play->state.closing   = 0U;
      rec_play->state.rw_active = 0U;
      rec_play->state.opening   = 1U;

      // Open sds stream (buffer) and sdsio stream (sds file).
      rec_play->sds_buffer = sdsBufferOpen(buf, buf_size, io_threshold, 0U);
      rec_play->sdsio = sdsioOpen(name, sdsioModeRead);

      if (rec_play->sds_buffer != NULL) {
        // Register event callback - low threshold.
        sdsBufferRegisterEvents(rec_play->sds_buffer, sdsRecPlayEventCallback, SDS_BUFFER_EVENT_DATA_LOW, (void *)index);
      }

      // Check if sds stream (buffer) and sdsio stream (sds file) were opened successfully.
      if ((rec_play->sds_buffer != NULL) && (rec_play->sdsio != NULL)) {
        // Streams were successfully opened.
        // Player stream is in the opening state and the sdsRecPlayThread should start
        // reading data from the SDS I/O interface into the SDS Stream Buffer.
        // Notify sdsRecPlayThread to process this stream by setting the corresponding thread flag.
        osThreadFlagsSet(sdsRecPlayThreadId, 1U << index);

        // Wait for notification from sdsRecPlayThread that the stream buffer is filled with the data.
        flags = osEventFlagsWait(sdsRecPlayOpenEventFlags, 1U << index, osFlagsWaitAll, SDS_REC_PLAY_OPEN_TOUT);
        if ((flags & osFlagsError) != 0U) {
          // Timeout or any other error occurred.
          err = 1U;
        }
      } else {
        err = 1U;
      }

      if (err == 0U) {
        // Streams were successfully opened, control block is initialized and SDS Stream Buffer is filled.
        // Update states: Stream is opened.
        rec_play->state.opened = 1U;
        rec_play->state.opening = 0U;
      } else {
        // Error occurred: Close streams and free control block.
        if (rec_play->sds_buffer != NULL) {
          sdsBufferClose(rec_play->sds_buffer);
          rec_play->sds_buffer = NULL;
        }
        if (rec_play->sdsio != NULL) {
          sdsioClose(rec_play->sdsio);
          rec_play->sdsio = NULL;
        }
        sdsRecPlayFree(index);
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
  uint32_t      event_mask, flags;

  if (sdsRecPlayInitialized == 0U) {
    // Recorder/Player is not initialized. Exit the function.
    return SDS_REC_PLAY_ERROR;
  }
  if (rec_play == NULL) {
    // Invalid stream. Exit the function.
    return SDS_REC_PLAY_ERROR;
  }
  if (rec_play->state.opened == 0U) {
    // Stream is not opened. Exit the function.
    return SDS_REC_PLAY_ERROR;
  }

  event_mask = 1 << rec_play->index;

  // Change state to closing.
  rec_play->state.closing = 1U;

  // Wait until reading from SDS Stream Buffer is completed.
  // Reading from SDS Stream Buffer (RAM) is expected to be fast, so this loop should not take long.
  while (rec_play->state.rw_active != 0U);

  // Before player stream is closed sdsRecPlayThread should stop reading data from SDS I/O interface.
  // Notify sdsRecPlayThread to process this stream by setting the corresponding thread flag.
  osThreadFlagsSet(sdsRecPlayThreadId, event_mask);

  // Wait for notification from sdsRecPlayThread that thread has stopped reading data.
  flags = osEventFlagsWait(sdsRecPlayCloseEventFlags, event_mask, osFlagsWaitAll, SDS_REC_PLAY_CLOSE_TOUT);
  if ((flags & osFlagsError) != 0U) {
    if (flags == osFlagsErrorTimeout) {
      // Timeout occurred.
      ret = SDS_REC_PLAY_ERROR_TIMEOUT;
    }
  } else {
    // Close streams and free control block.
    sdsBufferClose(rec_play->sds_buffer);
    sdsioClose(rec_play->sdsio);
    sdsRecPlayFree(rec_play->index);

    // Update states: Stream is closed.
    rec_play->state.opened  = 0U;
    rec_play->state.closing = 0U;

    ret = SDS_REC_PLAY_OK;
  }

  return ret;
}

// Read data from player stream.
uint32_t sdsPlayRead (sdsRecPlayId_t id, uint32_t *timestamp, void *buf, uint32_t buf_size) {
  sdsRecPlay_t *rec_play = id;
  uint32_t      num      = 0U;
  uint32_t      size;

  if (sdsRecPlayInitialized == 0U) {
    // Recorder/Player is not initialized. Exit the function.
    return 0U;
  }
  if (rec_play == NULL) {
    // Invalid stream. Exit the function.
    return 0U;
  }
  if ((rec_play->state.opened == 0U) || (rec_play->state.closing != 0U)) {
    // Stream is closing or already closed. Exit the function.
    return 0U;
  }

  // Set read/write active state.
  rec_play->state.rw_active = 1U;

  // Ensure the stream state has not transitioned to closing.
  if (rec_play->state.closing == 0U) {
    // Verify if parameters are valid.
    if ((buf != NULL) && (buf_size != 0U)) {

      // Get size of available data.
      size = sdsBufferGetCount(rec_play->sds_buffer);

      // Check if header was already read:
      // If not (head.data_size == 0) and there is enough data available, read new header
      if ((rec_play->head.data_size == 0U) && (size >= HEAD_SIZE)) {
        sdsBufferRead(rec_play->sds_buffer, &rec_play->head, HEAD_SIZE);
        size -= HEAD_SIZE;
      }

      if ((rec_play->head.data_size != 0U)   &&      // Check if Header is valid and data size in header is valid.
          (rec_play->head.data_size <= size) &&      // Check if whole record is available in the SDS Stream Buffer.
          (rec_play->head.data_size <= buf_size)) {  // Check if whole record fits into the provided buffer.

        // Read data from SDS Stream Buffer:
        // Buffer size has been validated, so read operation is expected to succeed.
        num = sdsBufferRead(rec_play->sds_buffer, buf, rec_play->head.data_size);

        // Get timestamp from the header.
        if (timestamp != NULL) {
          *timestamp = rec_play->head.timestamp;
        }

        // Whole record has been read from the SDS Stream Buffer.
        // Clear the header information to enable reading of the next record.
        rec_play->head.data_size = 0U;

        // If an SDS threshold event occurred during sdsRead (amount of data in the SDS Stream Buffer is at or below the threshold),
        // notify the sdsRecPlayThread by setting the corresponding thread flag to process the stream.
        if (rec_play->event_threshold != 0U) {
          rec_play->event_threshold = 0U;
          osThreadFlagsSet(sdsRecPlayThreadId, 1U << rec_play->index);
        }
      }
    }
  }
  // Clear read/write active state.
  rec_play->state.rw_active = 0U;

  return num;
}

// Get record data size from Player stream
uint32_t sdsPlayGetSize (sdsRecPlayId_t id) {
  sdsRecPlay_t *rec_play = id;
  uint32_t      size     = 0U;
  uint32_t      cnt;

  if (sdsRecPlayInitialized == 0U) {
    // Recorder/Player is not initialized. Exit the function.
    return 0U;
  }
  if (rec_play == NULL) {
    // Invalid stream. Exit the function.
    return 0U;
  }
  if ((rec_play->state.opened == 0U) || (rec_play->state.closing != 0U)) {
    // Stream is closing or already closed. Exit the function.
    return 0U;
  }

  // Set read/write active state.
  rec_play->state.rw_active = 1U;

  // Ensure the stream state has not transitioned to closing.
  if (rec_play->state.closing == 0U) {

    // Check if header was already read:
    // If not (head.data_size == 0) and there is enough data available, read new header.
    if (rec_play->head.data_size == 0U) {
      cnt = sdsBufferGetCount(rec_play->sds_buffer);
      if (cnt >= HEAD_SIZE) {
        sdsBufferRead(rec_play->sds_buffer, &rec_play->head, HEAD_SIZE);
      }
    }

    // Get data size from the header.
    size = rec_play->head.data_size;
  }
  // Clear read/write active state.
  rec_play->state.rw_active = 0U;

  return size;
}

// Check if end of stream has been reached
int32_t sdsPlayEndOfStream (sdsRecPlayId_t id) {
  sdsRecPlay_t *rec_play = id;
  int32_t       eos      = 0;

  if (sdsRecPlayInitialized == 0U) {
    // Recorder/Player is not initialized. Exit the function.
    return SDS_REC_PLAY_ERROR;
  }
  if (rec_play == NULL) {
    // Invalid stream. Exit the function.
    return 0U;
  }
  if ((rec_play->state.opened == 0U) || (rec_play->state.closing != 0U)) {
    // Stream is closing or already closed. Exit the function.
    return 0U;
  }

  // Set read/write active state.
  rec_play->state.rw_active = 1U;

  // Ensure the stream state has not transitioned to closing.
  if (rec_play->state.closing == 0U) {
    if (rec_play != NULL) {
      if ((sdsBufferGetCount(rec_play->sds_buffer) == 0U) && ((rec_play->flags & SDS_REC_PLAY_FLAG_EOS) != 0U)) {
        // SDS Stream Buffer is empty and SDS_REC_PLAY_FLAG_EOS flag is set. End of stream has been reached.
        eos = 1;
      }
    }
  }
  // Clear read/write active state.
  rec_play->state.rw_active = 0U;

  return eos;
}
