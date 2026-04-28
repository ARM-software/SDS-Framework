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

// Synchronous Data Stream (SDS)

#if !defined(__STDC_NO_ATOMICS__)
#include <stdatomic.h>
#endif
#include <string.h>

#include "cmsis_compiler.h"
#include "cmsis_os2.h"
#include "sds_config.h"
#include "sds.h"
#include "sds_buffer.h"
#include "sdsio.h"

#if (SDS_MAX_STREAMS > 31)
#error "Maximum number of concurrent SDS streams is 31!"
#endif

// Global error information
sdsError_t sdsError = { 0U, NULL, 0U, 0U };

// Global configuration options and diagnostic storage information (flags)
volatile uint32_t sdsFlags = 0U;

// Global state information
volatile uint32_t sdsState = SDS_STATE_INACTIVE;

// Global idle rate information
volatile uint32_t sdsIdleRate = 0xFFFFFFFF;

// Data block header
typedef struct {
  uint32_t    timeslot;         // Time slot value
  uint32_t    data_size;        // Size of a data block in bytes
} dataBlockHead_t;
#define HEAD_SIZE sizeof(dataBlockHead_t)

// SDS stream control block
typedef struct {
         uint8_t          index;            // Index of the SDS stream in psdsStreams array
         uint8_t          mode;             // Stream mode: SDS_STREAM_MODE_WRITE or SDS_STREAM_MODE_READ
volatile uint8_t          flags;            // Stream flags: SDS_STREAM_FLAG_ ..
         uint8_t          reserved;
volatile uint32_t         state;            // Stream state: SDS_STREAM_STATE_ ..
volatile uint32_t         lock;             // Lock for atomic operations
         uint32_t         buf_size;         // Size of the buffer used for the stream
         uint32_t         threshold;        // Threshold value
         sdsBufferId_t    sds_buffer;       // SDS Buffer stream handle
         sdsioId_t        sdsio;            // SDS I/O interface handle
         dataBlockHead_t  head;             // Data block header information
} sdsStream_t;

// Allocate memory for SDS streams depending on the configured maximum number of concurrent streams.
static sdsStream_t   sdsStreams[SDS_MAX_STREAMS] = {0};
static sdsStream_t *psdsStreams[SDS_MAX_STREAMS] = {NULL};

// Initialization flag
static uint8_t sdsInitialized = 0U;

// Data block buffer
static uint8_t sdsDataBlockBuf[SDS_BUF_SIZE];

// Event callback
static sdsEvent_t sdsEvent = NULL;

// Thread Resources
static const osThreadAttr_t sdsThreadAttr = {
  "sdsThread",
  osThreadDetached,
  NULL, 0, NULL, 
  SDS_THREAD_STACK_SIZE,
  SDS_THREAD_PRIORITY,
  0, 0
};
static osThreadId_t sdsThreadId;

// Close event flags
static osEventFlagsId_t sdsCloseEventFlags;

// Open event flags
static osEventFlagsId_t sdsOpenEventFlags;

// Event definitions
#define SDS_EVENT_FLAG_MASK             ((1UL << SDS_MAX_STREAMS) - 1)

// Flags definitions
#define SDS_STREAM_HALT                 (1U << 0)
#define SDS_STREAM_INITIAL_FILL         (1U << 1)
#define SDS_STREAM_EOS                  (1U << 2)

// Stream mode definitions
#define SDS_STREAM_MODE_WRITE           (1U << 0)
#define SDS_STREAM_MODE_READ            (1U << 1)

// Stream state definitions
#define SDS_STREAM_STATE_INACTIVE       0U // Data stream unused
#define SDS_STREAM_STATE_OPENING        1U // Open function is processed
#define SDS_STREAM_STATE_READ           2U // Data stream is in read mode
#define SDS_STREAM_STATE_WRITE          3U // Data stream is in write mode
#define SDS_STREAM_STATE_CLOSING        4U // Close function is processed

// Helper functions:

// Atomic Operation: Write 32-bit value to memory if existing value in memory is zero.
//  Return: 1 when new value is written or 0 otherwise.
#if defined(__STDC_NO_ATOMICS__) || !defined(ATOMIC_CHAR32_T_LOCK_FREE) || (ATOMIC_CHAR32_T_LOCK_FREE < 2)
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

static uint32_t sdsLockAcquire (sdsStream_t *stream, uint32_t timeout) {
  uint32_t *pLock = (uint32_t *)&stream->lock;
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

static void sdsLockRelease (sdsStream_t *stream) {
  stream->lock = 0U;
}

static sdsStream_t * sdsAlloc (uint32_t *index) {
  sdsStream_t    *stream = NULL;
  static uint32_t idx = 0U;
  uint32_t        n;

  for (n = 0U; n < SDS_MAX_STREAMS; n++) {
    if (atomic_wr32_if_zero((uint32_t *)&psdsStreams[idx], (uint32_t)&sdsStreams[idx]) != 0U) {
      stream = &sdsStreams[idx];
      if (index != NULL) {
        *index = idx;
      }
      break;
    }
    idx++;
    if (idx >= SDS_MAX_STREAMS) {
      idx = 0U;
    }
  }
  return stream;
}

static void sdsFree (uint32_t index) {
  psdsStreams[index] = NULL;
}

// Write Handler.
static void sdsWriteHandler (sdsStream_t *stream) {
  uint32_t bytes_remaining, bytes_to_transfer, bytes_transferred, state;
  int32_t  sdsio_ret;

  if ((stream->flags & SDS_STREAM_HALT) != 0U) {
    // The state of the stream is either closing or inactive.
    // Closing state has been processed by sdsThread. Exit the function.
    return;
  }

  state = stream->state;

  // Get number of data bytes in the stream buffer.
  bytes_remaining = sdsBufferGetCount(stream->sds_buffer);

  while (bytes_remaining != 0U) {
    bytes_to_transfer = bytes_remaining;
    if (bytes_to_transfer > sizeof(sdsDataBlockBuf)) {
      // Adjust number of bytes to transfer to size of intermediate buffer, so that data fits into the buffer.
      bytes_to_transfer = sizeof(sdsDataBlockBuf);
    }

    // Read data from the SDS Stream Buffer to intermediate buffer.
    sdsBufferRead(stream->sds_buffer, sdsDataBlockBuf, bytes_to_transfer);

    // Write data from intermediate buffer to SDS I/O Interface.
    sdsio_ret = sdsioWrite(stream->sdsio, sdsDataBlockBuf, bytes_to_transfer);
    if (sdsio_ret >= 0) {
      // Number of bytes written to the SDS I/O interface
      bytes_transferred = sdsio_ret;
    } else {
      if (sdsEvent != NULL) {
        // Notify the application about I/O error.
        sdsEvent(stream, SDS_EVENT_ERROR_IO);
      }
      break;
    }

    // Update remaining bytes to be transferred.
    if (state != SDS_STREAM_STATE_CLOSING) {
      bytes_remaining -= bytes_transferred;
    } else {
      // State of the stream is closing. Transfer all available data.
      bytes_remaining = sdsBufferGetCount(stream->sds_buffer);
    }
  }

  if (state == SDS_STREAM_STATE_CLOSING) {
    // State of the stream is closing.
    // All data has been successfully read from the stream buffer and written to the SDS I/O interface.

    // Set the internal SDS_STREAM_HALT flag to mark that sdsThread has finished processing the closing state.
    stream->flags |= SDS_STREAM_HALT;
    // Notify the thread waiting for the event in the sdsClose function to finalize the closing of the write stream.
    osEventFlagsSet(sdsCloseEventFlags, 1U << stream->index);
  }
}

// Read Handler.
static void sdsReadHandler (sdsStream_t *stream) {
  uint32_t bytes_remaining, bytes_to_transfer, bytes_transferred;
  int32_t  sdsio_ret;

  if ((stream->flags & SDS_STREAM_HALT) != 0U) {
    // State of the stream is closing or inactive.
    // Closing state has already been processed by sdsThread. Exit the function.
    return;
  }
  if (stream->state == SDS_STREAM_STATE_CLOSING) {
    // State of the stream is closing. Thread sdsThread has stopped reading data from SDS I/O interface.
    // Set the internal SDS_STREAM_HALT flag to mark that sdsThread has finished processing the closing state.
    stream->flags |= SDS_STREAM_HALT;
    // Notify the thread waiting for the event in the sdsClose function to finalize the closing of the read stream.
    osEventFlagsSet(sdsCloseEventFlags, 1U << stream->index);
    return;
  }

  // Calculate available space in the stream buffer.
  bytes_remaining = stream->buf_size - sdsBufferGetCount(stream->sds_buffer);
  while (bytes_remaining != 0U) {
    bytes_to_transfer = bytes_remaining;
    if (bytes_to_transfer > sizeof(sdsDataBlockBuf)) {
      // Adjust number of bytes to transfer to size of intermediate buffer, so that data fits into the buffer.
      bytes_to_transfer = sizeof(sdsDataBlockBuf);
    }

    // Read data from the SDS I/O Interface to intermediate buffer.
    sdsio_ret = sdsioRead(stream->sdsio, sdsDataBlockBuf, bytes_to_transfer);
    if (sdsio_ret > 0) {
      // Number of bytes read from the SDS I/O interface
      bytes_transferred = sdsio_ret;
    } else if (sdsio_ret == SDS_EOS) {
      // End of stream reached.
      bytes_transferred = 0U;
      stream->flags |= SDS_STREAM_EOS;
    } else {
      // Error occurred during reading from the SDS I/O interface.
      if (sdsEvent != NULL) {
        // Notify the application about I/O error.
        sdsEvent(stream, SDS_EVENT_ERROR_IO);
      }
      break;
    }

    if (bytes_transferred != 0U) {
      // Write data from intermediate buffer to the SDS Stream Buffer.
      // Available space in stream buffer has been validated, so write operation is expected to succeed.
      sdsBufferWrite(stream->sds_buffer, sdsDataBlockBuf, bytes_transferred);
    }

    // Check if the stream is in the opening state and the processing of the state is not completed.
    if ((stream->state == SDS_STREAM_STATE_OPENING) && ((stream->flags & SDS_STREAM_INITIAL_FILL) == 0U)) {
      // Check if SDS Stream Buffer is filled with data from the SDS I/O interface (at least to threshold or EOS).
      if ((sdsBufferGetCount(stream->sds_buffer) >= stream->threshold) || ((stream->flags & SDS_STREAM_EOS) != 0U)) {
        // Stream buffer is filled with data from the SDS I/O interface.
        // Set the internal SDS_STREAM_INITIAL_FILL flag to mark that sdsReadHandler has finished processing the opening state.
        stream->flags |= SDS_STREAM_INITIAL_FILL;
        // Notify the thread waiting for the event in the sdsOpen function to finalize the opening of the read stream.
        osEventFlagsSet(sdsOpenEventFlags, 1U << stream->index);
      }
    }

    // Update remaining bytes to transfer.
    bytes_remaining -= bytes_transferred;

    if ((stream->flags & SDS_STREAM_EOS) != 0U) {
      // End of stream reached. Exiting loop.
      break;
    }
  }
}

// SDS system thread.
static __NO_RETURN void sdsThread (void *arg) {
  sdsStream_t *stream;
  uint32_t     flags, n;

  (void)arg;

  while (1) {
    // Wait for threshold event flags.
    flags = osThreadFlagsWait(SDS_EVENT_FLAG_MASK, osFlagsWaitAny, osWaitForever);
    if ((flags & osFlagsError) == 0U) {
      // Process all streams with set flags.
      for (n = 0U; n < SDS_MAX_STREAMS; n++) {
        if ((flags & (1U << n)) == 0U) {
          // The flag is not set for this stream. Moving to the next stream.
          continue;
        }
        // Get stream control block.
        stream = psdsStreams[n];
        if (stream == NULL) {
          // Stream control block is not allocated. Move to next stream.
          continue;
        }
        if (stream->state == SDS_STREAM_STATE_INACTIVE) {
          // Stream is inactive. Move to the next stream.
          continue;
        }

        if (stream->mode == SDS_STREAM_MODE_WRITE) {
          // Write stream.
          sdsWriteHandler(stream);
        } else if (stream->mode == SDS_STREAM_MODE_READ) {
          // Read stream.
          sdsReadHandler(stream);
        }
      }
    }
  }
}

/**
  Initialize SDS system.
*/
int32_t sdsInit (sdsEvent_t event_cb) {
  int32_t ret = SDS_OK;

  if (sdsInitialized != 0U) {
    // The SDS system is already initialized.
    return SDS_OK;
  }

  // Reset an array of pointers to stream control blocks.
  memset(psdsStreams, 0, sizeof(psdsStreams));

  // Initialize SDS I/O interface.
  ret = sdsioInit();

  // Create SDS system thread.
  if (ret == SDS_OK) {
    sdsThreadId = osThreadNew(sdsThread, NULL, &sdsThreadAttr);
    if (sdsThreadId == NULL) {
      ret = SDS_ERROR;
    }
  }

  // Create open event flags.
  if (ret == SDS_OK) {
    sdsOpenEventFlags = osEventFlagsNew(NULL);
    if (sdsOpenEventFlags == NULL) {
      SDS_PRINTF("SDS initialization failed to create necessary event flags!\n");
      ret = SDS_ERROR;
    }
  }

  // Create close event flags.
  if (ret == SDS_OK) {
    sdsCloseEventFlags = osEventFlagsNew(NULL);
    if (sdsCloseEventFlags == NULL) {
      SDS_PRINTF("SDS initialization failed to create necessary event flags!\n");
      ret = SDS_ERROR;
    }
  }

  // Set event callback.
  if (ret == SDS_OK) {
    // Initialization successful.
    sdsEvent = event_cb;
    sdsInitialized = 1U;
  } else {
    // Initialization failed:
    // Terminate thread, delete event flags, and uninitialize SDS I/O interface.
    if (sdsThreadId != NULL) {
      osThreadTerminate(sdsThreadId);
    }
    if (sdsOpenEventFlags != NULL) {
      osEventFlagsDelete(sdsOpenEventFlags);
    }
    if (sdsCloseEventFlags != NULL) {
      osEventFlagsDelete(sdsCloseEventFlags);
    }
    sdsioUninit();
  }

  return ret;
}

/**
  Uninitialize SDS system.
*/
int32_t sdsUninit (void) {

  if (sdsInitialized == 0U) {
    // SDS system not yet initialized.
    return SDS_OK;
  }

  // Clear initialization flag.
  sdsInitialized = 0U;

  // Terminate thread and delete event flags.
  osThreadTerminate(sdsThreadId);
  osEventFlagsDelete(sdsOpenEventFlags);
  osEventFlagsDelete(sdsCloseEventFlags);

  // Clear event callback.
  sdsEvent = NULL;

  // Uninitialize SDS I/O interface.
  sdsioUninit();

  return SDS_OK;
}

// SDS functions

/**
  Open SDS stream.
*/
sdsId_t sdsOpen (const char *name, sdsMode_t mode, void *buf, uint32_t buf_size) {
  sdsStream_t *stream = NULL;
  uint32_t     index, flags;
  int32_t      err = SDS_OK;

  if (sdsInitialized == 0U) {
    // The SDS system is not initialized. Exiting the function.
    return NULL;
  }

  if ((name == NULL) || (buf == NULL) || (buf_size == 0U)) {
    // If any parameter is invalid. Exit the function.
    return NULL;
  }

  // Atomic allocation of a new control block for the SDS stream.
  stream = sdsAlloc(&index);
  if (stream == NULL) {
    // If new control block allocation failed. Exit the function.
    return NULL;
  }

  if (sdsLockAcquire(stream, SDS_OPEN_TIMEOUT) == 0U) {
    // Timeout occurred while waiting for lock. Free control block and exit the function.
    sdsFree(index);
    return NULL;
  }

  // Set control block parameters.
  stream->state           = SDS_STREAM_STATE_INACTIVE;
  stream->index           = index & 0xFFU;
  stream->flags           = 0U;
  stream->buf_size        = buf_size;
  stream->head.timeslot   = 0U;
  stream->head.data_size  = 0U;

  // Set threshold value for the stream.
  if ((buf_size / 3) < SDS_IO_TRANSFER_SIZE) {
    // Set threshold to 1/3 of the buffer size.
    stream->threshold = buf_size / 3;
  } else {
    // Set threshold to SDS I/O interface efficient transfer size.
    stream->threshold = SDS_IO_TRANSFER_SIZE;
  }

  // Open stream buffer.
  stream->sds_buffer = sdsBufferOpen(buf, buf_size, 0U, 0U);

  if (mode == sdsModeWrite) {           // Write mode
    stream->mode = SDS_STREAM_MODE_WRITE;

    // Open sdsio stream (sds file).
    stream->sdsio = sdsioOpen(name, sdsioModeWrite);

    // Check if sds stream (buffer) and sdsio stream (sds file) were opened successfully.
    if ((stream->sds_buffer != NULL) && (stream->sdsio != NULL)) {
      // Stream was successfully opened and control block is initialized.
      // Set state to write.
      stream->state = SDS_STREAM_STATE_WRITE;
    } else {
      err = SDS_ERROR;
    }
  } else {                              // Read mode
    stream->mode = SDS_STREAM_MODE_READ;

    // Open sdsio stream (sds file).
    stream->sdsio = sdsioOpen(name, sdsioModeRead);

    // Check if sds stream (buffer) and sdsio stream (sds file) were opened successfully.
    if ((stream->sds_buffer != NULL) && (stream->sdsio != NULL)) {
      // Streams were successfully opened. Set state to opening.
      stream->state = SDS_STREAM_STATE_OPENING;
      // Read stream is in the opening state and the sdsThread should start
      // reading data from the SDS I/O interface into the SDS Stream Buffer.
      // Notify sdsThread to process this stream by setting the corresponding thread flag.
      osThreadFlagsSet(sdsThreadId, 1U << index);

      // Wait for notification from sdsThread that the stream buffer is filled with the data.
      flags = osEventFlagsWait(sdsOpenEventFlags, 1U << index, osFlagsWaitAll, SDS_OPEN_TIMEOUT);
      if ((flags & osFlagsError) != 0U) {
        // Timeout or any other error occurred.
        err = SDS_ERROR;
      }
    } else {
      err = SDS_ERROR;
    }

    if (err == SDS_OK) {
      // Stream was successfully opened, control block is initialized and SDS Stream Buffer is filled.
      // Set state to read.
      stream->state = SDS_STREAM_STATE_READ;
    }
  }

  if (err != SDS_OK) {
    // Error occurred: Close streams, reset state.
    if (stream->sds_buffer != NULL) {
      sdsBufferClose(stream->sds_buffer);
      stream->sds_buffer = NULL;
    }
    if (stream->sdsio != NULL) {
      sdsioClose(stream->sdsio);
      stream->sdsio = NULL;
    }
    stream->state = SDS_STREAM_STATE_INACTIVE;
  }

  sdsLockRelease(stream);

  if (err != SDS_OK) {
    // Error occurred: free control block.
    sdsFree(index);
    stream = NULL;
  }

  return stream;
}

/**
  Close SDS stream.
*/
int32_t sdsClose (sdsId_t id) {
  sdsStream_t *stream = id;
  uint32_t     state;
  int32_t      ret      = SDS_ERROR;
  int32_t      err      = SDS_OK;
  uint32_t     event_mask, flags;

  if (sdsInitialized == 0U) {
    // The SDS system is not initialized. Exiting the function.
    return SDS_ERROR;
  }
  if (stream == NULL) {
    // Invalid stream. Exit the function.
    return SDS_ERROR_PARAMETER;
  }
  if (sdsLockAcquire(stream, SDS_CLOSE_TIMEOUT) == 0U) {
    // Timeout occurred while waiting for lock.
    return SDS_ERROR_TIMEOUT;
  }
  if (((stream->mode == SDS_STREAM_MODE_WRITE) && (stream->state != SDS_STREAM_STATE_WRITE)) ||
      ((stream->mode == SDS_STREAM_MODE_READ ) && (stream->state != SDS_STREAM_STATE_READ))) {
    // Stream is not in expected mode. Exit the function.
    sdsLockRelease(stream);
    return SDS_ERROR;
  }

  // Store current SDS Stream state
  state = stream->state;

  // Set state to closing.
  stream->state = SDS_STREAM_STATE_CLOSING;

  // Before SDS stream is closed, sdsThread should send all data in SDS Stream Buffer via SDS I/O interface for write mode,
  // for read mode sdsThread should stop reading data from SDS I/O interface.
  // Notify sdsThread to process this stream by setting the corresponding thread flag.
  event_mask = 1U << stream->index;
  osThreadFlagsSet(sdsThreadId, event_mask);

  // Wait for notification from sdsThread that thread has transferred all data from SDS Stream Buffer for write mode,
  // for read mode wait for notification from sdsThread that thread has stopped reading data.
  flags = osEventFlagsWait(sdsCloseEventFlags, event_mask, osFlagsWaitAll, SDS_CLOSE_TIMEOUT);
  if ((flags & osFlagsError) != 0U) {
    if (flags == osFlagsErrorTimeout) {
      // Timeout occurred.
      err = SDS_ERROR_TIMEOUT;
    } else {
      err = SDS_ERROR;
    }
  }

  if (err == SDS_OK) {
    // Close SDSIO stream.
    err = sdsioClose(stream->sdsio);
  }

  if (err == SDS_OK) {
    // Close SDS Buffer.
    sdsBufferClose(stream->sds_buffer);

    // Reset control block parameters.
    stream->sds_buffer = NULL;
    stream->sdsio      = NULL;

    // Set state to inactive.
    stream->state = SDS_STREAM_STATE_INACTIVE;

    sdsFree(stream->index);

    ret = SDS_OK;
  } else {
    // Close failed. Restore state back.
    stream->state = state;
    ret = err;
  }
  // Release lock.
  sdsLockRelease(stream);

  return ret;
}

/**
  Write entire data block along with its time slot information to the SDS stream opened in write mode.
*/
int32_t sdsWrite (sdsId_t id, uint32_t timeslot, const void *buf, uint32_t buf_size) {
  sdsStream_t    *stream = id;
  int32_t         ret      = SDS_ERROR;
  dataBlockHead_t head;

  if (sdsInitialized == 0U) {
    // The SDS system is not initialized. Exiting the function.
    return SDS_ERROR;
  }
  if (stream == NULL) {
    // Invalid stream. Exit the function.
    return SDS_ERROR_PARAMETER;
  }
  if (sdsLockAcquire(stream, 0) == 0U) {
    // Failed to acquire lock.
    return SDS_ERROR;
  }
  if (stream->state != SDS_STREAM_STATE_WRITE) {
    // Stream is not in writing state. Exit the function.
    sdsLockRelease(stream);
    return SDS_ERROR;
  }

  // Verify if parameters are valid.
  if ((buf != NULL) && (buf_size != 0U)) {

    // Check if header + data fits into the buffer.
    if ((buf_size + sizeof(dataBlockHead_t)) <= (stream->buf_size - sdsBufferGetCount(stream->sds_buffer))) {
      // Header: time slot, data block size.
      head.timeslot  = timeslot;
      head.data_size = buf_size;

      // Write header and data block: Buffer size has been validated, so write operations are expected to succeed.
      sdsBufferWrite(stream->sds_buffer, &head, sizeof(dataBlockHead_t));
      ret = sdsBufferWrite(stream->sds_buffer, buf, buf_size);

      // If amount of data in the SDS Stream Buffer is at or above the threshold,
      // notify the sdsThread by setting the corresponding thread flag to process the stream.
      if (sdsBufferGetCount(stream->sds_buffer) >= stream->threshold) {
        osThreadFlagsSet(sdsThreadId, 1U << stream->index);
      }
    } else {
      // Insufficient space in the stream buffer.
      if (sdsEvent != NULL) {
        // Notify the application about the error.
        sdsEvent(stream, SDS_EVENT_NO_SPACE);
      }
      ret = SDS_NO_SPACE;
    }
  } else {
    ret = SDS_ERROR_PARAMETER;
  }

  // Release lock.
  sdsLockRelease(stream);

  return ret;
}

/**
  Read entire data block along with its time slot information from the SDS stream opened in read mode.
*/
int32_t sdsRead (sdsId_t id, uint32_t *timeslot, void *buf, uint32_t buf_size) {
  sdsStream_t *stream = id;
  int32_t      ret      = SDS_ERROR;
  int32_t      eos      = 0;
  int32_t      size;

  if (sdsInitialized == 0U) {
    // The SDS system is not initialized. Exiting the function.
    return SDS_ERROR;
  }
  if (stream == NULL) {
    // Invalid stream. Exit the function.
    return SDS_ERROR_PARAMETER;
  }
  if (sdsLockAcquire(stream, 0) == 0U) {
    // Failed to acquire lock.
    return SDS_ERROR;
  }
  if (stream->state != SDS_STREAM_STATE_READ) {
    // Stream is not in read state. Exit the function.
    sdsLockRelease(stream);
    return SDS_ERROR;
  }

  // Verify if parameters are valid.
  if ((buf != NULL) && (buf_size != 0U)) {
    // Save eos flag.
    eos = stream->flags & SDS_STREAM_EOS;
    // Get size of available data block.
    size = sdsBufferGetCount(stream->sds_buffer);

    // Check if header was already read:
    // If not (head.data_size == 0) and there is enough data available, read new header.
    if ((stream->head.data_size == 0U) && (size >= HEAD_SIZE)) {
      sdsBufferRead(stream->sds_buffer, &stream->head, HEAD_SIZE);
      size -= HEAD_SIZE;
    }

    // Check if Header is valid and data block size in header is valid.
    if (stream->head.data_size != 0U) {
      // Check if whole data block fits into the provided buffer.
      if (stream->head.data_size > buf_size) {
        // Provided buffer is too small to read the data block.
        ret = SDS_ERROR_PARAMETER;
      } else {
        // Check if whole data block is available in the SDS Stream Buffer.
        if (stream->head.data_size > size) {
          // Whole data block is not available in the SDS Stream Buffer.
          ret = SDS_NO_DATA;
        } else {
          // Read data block from SDS Stream Buffer:
          // Buffer size has been validated, so read operation is expected to succeed.
          ret = sdsBufferRead(stream->sds_buffer, buf, stream->head.data_size);

          // Get time slot from the header.
          if (timeslot != NULL) {
            *timeslot = stream->head.timeslot;
          }

          // Whole data block has been read from the SDS Stream Buffer.
          // Clear the header information to enable reading of the next data block.
          stream->head.data_size = 0U;

          // If free space in the SDS Stream Buffer is at or above the threshold,
          // notify the sdsThread by setting the corresponding thread flag to process the stream.
          if ((stream->buf_size - sdsBufferGetCount(stream->sds_buffer)) >= stream->threshold) {
            osThreadFlagsSet(sdsThreadId, 1U << stream->index);
          }
        }
      }
    } else {
      // Header is not valid.
      // Check for End Of Stream.
      if ((size == 0) && (eos != 0U)) {
        ret = SDS_EOS;
      } else {
        // Whole header is not available in the SDS Stream Buffer.
        ret = SDS_NO_DATA;
      }
    }
  } else {
    ret = SDS_ERROR_PARAMETER;
  }

  if ((ret == SDS_NO_DATA) && (sdsEvent != NULL)) {
    // Notify the application about the error.
    sdsEvent(stream, SDS_EVENT_NO_DATA);
  }

  // Release Lock.
  sdsLockRelease(stream);

  return ret;
}

/**
  Get data block size from an SDS stream opened in read mode.
*/
int32_t sdsGetSize (sdsId_t id) {
  sdsStream_t *stream = id;
  int32_t      ret      = SDS_ERROR;
  int32_t      eos      = 0;
  int32_t      size;

  if (sdsInitialized == 0U) {
    // The SDS system is not initialized. Exiting the function.
    return SDS_ERROR;
  }
  if (stream == NULL) {
    // Invalid stream. Exit the function.
    return SDS_ERROR_PARAMETER;
  }
  if (sdsLockAcquire(stream, 0) == 0U) {
    // Failed to acquire lock.
    return SDS_ERROR;
  }
  if (stream->state != SDS_STREAM_STATE_READ) {
    // Stream is not in read state. Exit the function.
    sdsLockRelease(stream);
    return SDS_ERROR;
  }

  // Save eos flag.
  eos = stream->flags & SDS_STREAM_EOS;
  // Get size of available data block.
  size = sdsBufferGetCount(stream->sds_buffer);

  // Check if header was already read:
  // If not (head.data_size == 0) and there is enough data available, read new header.
  if ((stream->head.data_size == 0U) && (size >= HEAD_SIZE)) {
    sdsBufferRead(stream->sds_buffer, &stream->head, HEAD_SIZE);
    size -= HEAD_SIZE;
  }

  // Check if Header is valid and data block size in header is valid.
  if (stream->head.data_size != 0U) {
    if (stream->head.data_size > size) {
      // Whole data block is not available in the SDS Stream Buffer.
      ret = SDS_NO_DATA;
    } else {
      // Return size of the data block.
      ret = stream->head.data_size;
    }
  } else {
    // Header is not valid.
    // Check for End Of Stream.
    if ((size == 0) && (eos != 0U)) {
      ret = SDS_EOS;
    } else {
      // Whole header is not available in the SDS Stream Buffer.
      ret = SDS_NO_DATA;
    }
  }

  // Release Lock.
  sdsLockRelease(stream);
  return ret;
}
