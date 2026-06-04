/*
 * Copyright (c) 2023-2026 Arm Limited. All rights reserved.
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

// SDSIO via File System (Keil::File System)

#include <string.h>
#include <stdio.h>

#include "cmsis_os2.h"

#include "rl_fs.h"                      // Keil.MDK-Plus::File System:CORE
#include "sds.h"
#include "sdsio.h"
#include "sdsio_fs_mdk_config.h"

// Maximum stream name length
#ifndef SDSIO_MAX_NAME_SIZE
#define SDSIO_MAX_NAME_SIZE         32U
#endif

// Max length of index and file extension (.NNN.p.sds.bak)
#define SDSIO_MAX_EXT_SIZE          20U

// Buffer for file name construction (shared to avoid stack usage in sdsioOpen)
static char file_name[sizeof(SDSIO_WORK_DIR) + SDSIO_MAX_NAME_SIZE + SDSIO_MAX_EXT_SIZE];
static char bak_name [sizeof(SDSIO_WORK_DIR) + SDSIO_MAX_NAME_SIZE + SDSIO_MAX_EXT_SIZE];

// Playback mode flag
static uint32_t sdsio_playback_flag = 0U;

#define SDSIO_INVALID_INDEX 0xFFFFFFFFU

// Session state (reset on sdsioInit)
static uint32_t sdsio_rec_index   = SDSIO_INVALID_INDEX;  // shared recording index for the current session
static uint32_t sdsio_play_index  = SDSIO_INVALID_INDEX;  // shared playback index for the current session
static uint32_t sdsio_index_valid = 0U;                   // flag indicating that session index has been determined and is valid
static uint32_t sdsio_open_cnt    = 0U;                   // number of currently open streams

// Lock function
#ifndef SDSIO_FS_NO_LOCK

#ifndef SDSIO_FS_LOCK_TIMEOUT
#define SDSIO_FS_LOCK_TIMEOUT       5000U
#endif

static osMutexId_t lock_id;
static inline int32_t sdsioLockCreate (void) {
  lock_id = osMutexNew(NULL);
  if (lock_id != NULL) {
    return SDS_OK;
  } else {
    return SDS_ERROR_IO;
  }
}
static inline int32_t sdsioLockDelete (void) {
  osMutexDelete(lock_id);
  return SDS_OK;
}
static inline int32_t sdsioLock (void) {
  osStatus_t  status;

  status = osMutexAcquire(lock_id, SDSIO_FS_LOCK_TIMEOUT);
  if (status == osOK) {
    return SDS_OK;
  } else if (status == osErrorTimeout) {
    return SDS_ERROR_TIMEOUT;
  } else {
    return SDS_ERROR_IO;
  }

}
static inline int32_t sdsioUnlock (void) {
  osMutexRelease(lock_id);
  return SDS_OK;
}
#else
static inline int32_t sdsioLockCreate (void) { return SDS_OK; }
static inline int32_t sdsioLockDelete (void) { return SDS_OK; }
static inline int32_t sdsioLock       (void) { return SDS_OK; }
static inline int32_t sdsioUnlock     (void) { return SDS_OK; }
#endif

// SDSIO functions

/**
  \fn          int32_t sdsioInit (void)
  \brief       Initialize SDSIO interface.
  \return      SDS_OK on success or
               a negative value on error (see \ref SDS_Return_Codes)
*/
int32_t sdsioInit (void) {
  int32_t  ret = SDS_ERROR_IO;
  uint32_t stat;

  if (sdsioLockCreate() != SDS_OK) {
    return SDS_ERROR_IO;
  }

  // Initialize and mount file system drive
  stat = finit(SDSIO_DRIVE);
  if (stat == fsOK) {
    stat = fmount(SDSIO_DRIVE);
#if (SDSIO_FORMATTING_ALLOWED == 1)
    if (stat == fsNoFileSystem) {
      stat = fformat(SDSIO_DRIVE, NULL);
    }
#endif
  }

  if (stat == fsOK) {
    sdsio_rec_index     = SDSIO_INVALID_INDEX;
    sdsio_play_index    = SDSIO_INVALID_INDEX;
    sdsio_index_valid   = 0U;
    sdsio_playback_flag = 0U;
    sdsio_open_cnt      = 0U;

    SDS_PRINTF("SDSIO File System (MDK-FS) interface initialized successfully\n");
    ret = SDS_OK;
  } else {
    sdsioLockDelete();
    SDS_PRINTF("SDSIO File System MDK-FS interface initialization failed!\n");
  }

  return ret;
}

/**
  \fn          int32_t sdsioUninit (void)
  \brief       Un-initialize SDSIO interface.
  \return      SDS_OK on success or
               a negative value on error (see \ref SDS_Return_Codes)
*/
int32_t sdsioUninit (void) {
  funmount(SDSIO_DRIVE);
  funinit(SDSIO_DRIVE);
  sdsioLockDelete();
  return SDS_OK;
}

/**
  \fn          sdsioId_t sdsioOpen (const char *name, sdsioMode_t mode)
  \brief       Open SDSIO stream.
  \param[in]   name           stream name (pointer to NULL terminated string)
  \param[in]   mode           \ref sdsioMode_t open mode
  \return      \ref sdsioId_t Handle to SDSIO stream, or NULL if operation failed
*/
sdsioId_t sdsioOpen (const char *name, sdsioMode_t mode) {
  FILE     *file = NULL;
  uint32_t  index;

  if (name == NULL) {
    SDS_PRINTF("SDSIO: Stream name is NULL\n");
    return NULL;
  }

  if (strlen(name) > SDSIO_MAX_NAME_SIZE) {
    SDS_PRINTF("SDSIO: Stream name '%s' is too long (max %i characters)\n", name, SDSIO_MAX_NAME_SIZE);
    return NULL;
  }

  if (sdsioLock() != SDS_OK) {
    return NULL;
  }

  if (sdsio_open_cnt == 0U) {
    // Mode can only be determined at session start (when no stream is open).
    // Once a stream is open, the mode is fixed for the session.
    sdsio_playback_flag = (sdsFlags & SDS_FLAG_PLAYBACK) != 0U;
  }

  if ((mode == sdsioModeRead) && (sdsio_playback_flag == 0U)) {
    SDS_PRINTF("SDSIO: Cannot open stream '%s' for playback. Playback mode is not enabled.\n", name);
  } else {

    if (sdsio_index_valid == 0U) {
      // Session index not determined yet: determine next index
      if (sdsio_playback_flag != 0U) {
        if (sdsio_play_index != SDSIO_INVALID_INDEX) {
          sdsio_play_index++;
        } else {
          sdsio_play_index = 0U;
        }
      } else {
        if (sdsio_rec_index != SDSIO_INVALID_INDEX) {
          sdsio_rec_index++;
        } else {
          sdsio_rec_index = 0U;
          do {
            sprintf(file_name, "%s%s.%i.sds", SDSIO_WORK_DIR, name, sdsio_rec_index);
            file = fopen(file_name, "rb");
            if (file != NULL) {
              fclose(file);
              file = NULL;
              sdsio_rec_index++;
            } else {
              break;
            }
          } while (1U);
        }
      }
      sdsio_index_valid   = 1U;
    }

    if (sdsio_playback_flag != 0U) {
      index = sdsio_play_index;
    } else {
      index = sdsio_rec_index;
    }

    switch (mode) {
      case sdsioModeRead:
        sprintf(file_name, "%s%s.%i.sds", SDSIO_WORK_DIR, name, index);
        file = fopen(file_name, "rb");
        break;

      case sdsioModeWrite:
        if (sdsio_playback_flag != 0U) {
          // In playback mode: file name = *.p.sds
          sprintf(file_name, "%s%s.%i.p.sds", SDSIO_WORK_DIR, name, index);
        } else {
          // In recording mode: file name = *.sds
          sprintf(file_name, "%s%s.%i.sds", SDSIO_WORK_DIR, name, index);
        }

        // Check if file already exists
        file = fopen(file_name, "rb");
        if (file != NULL) {
          // File exists: back up existing file before overwriting
          fclose(file);
          sprintf(bak_name, "%s.bak", file_name);
          fdelete(bak_name, NULL);
          frename(file_name, bak_name);
        }
        file = fopen(file_name, "wb");
        break;
    }

    if (file != NULL) {
      sdsio_open_cnt++;
    }
  }

  sdsioUnlock();

  return (sdsioId_t)file;
}

/**
  \fn          int32_t sdsioClose (sdsioId_t id)
  \brief       Close SDSIO stream.
  \param[in]   id             \ref sdsioId_t handle to SDSIO stream
  \return      SDS_OK on success or
               a negative value on error (see \ref SDS_Return_Codes)
*/
int32_t sdsioClose (sdsioId_t id) {
  FILE   *file = (FILE *)id;
  int32_t ret;

  if (file == NULL) {
    return SDS_ERROR_PARAMETER;
  }

  ret = sdsioLock();
  if (ret == SDS_OK) {
    if (fclose(file) == 0) {
      ret = SDS_OK;
      if (sdsio_open_cnt > 0U) {
        sdsio_open_cnt--;
        if (sdsio_open_cnt == 0U) {
          // All streams closed: advance to next session index
          sdsio_index_valid = 0U;
        }
      }
    } else {
      ret = SDS_ERROR_IO;
    }
    sdsioUnlock();
  }

  return ret;
}

/**
  \fn          int32_t sdsioWrite (sdsioId_t id, const void *buf, uint32_t buf_size)
  \brief       Write data to SDSIO stream.
  \param[in]   id             \ref sdsioId_t handle to SDSIO stream
  \param[in]   buf            pointer to buffer with data to write
  \param[in]   buf_size       buffer size in bytes
  \return      number of bytes successfully written or
               a negative value on error (see \ref SDS_Return_Codes)
*/
int32_t sdsioWrite (sdsioId_t id, const void *buf, uint32_t buf_size) {
  FILE    *file = (FILE *)id;
  int32_t  ret  = SDS_ERROR_IO;
  uint32_t num;

  num = fwrite(buf, 1, buf_size, file);
  if (num < buf_size) {
    ret = SDS_ERROR_IO;
  } else {
    ret = (int32_t)num;
  }

  return ret;
}

/**
  \fn          int32_t sdsioRead (sdsioId_t id, void *buf, uint32_t buf_size)
  \brief       Read data from SDSIO stream.
  \param[in]   id             \ref sdsioId_t handle to SDSIO stream
  \param[out]  buf            pointer to buffer for data to read
  \param[in]   buf_size       buffer size in bytes
  \return      number of bytes successfully read, or
               a negative value on error or SDS_EOS (see \ref SDS_Return_Codes)
*/
int32_t sdsioRead (sdsioId_t id, void *buf, uint32_t buf_size) {
  FILE    *file = (FILE *)id;
  int32_t  ret  = SDS_ERROR_IO;
  uint32_t num;

  num = fread(buf, 1, buf_size, file);
  if (num > 0U) {
    if ((num < buf_size) && (ferror(file) != 0)) {
      // Error happened
      ret = SDS_ERROR_IO;
    } else {
      ret = (int32_t)num;
    }
  } else {
    if (feof(file) != 0) {
      // End of stream reached
      ret = SDS_EOS;
    } else {
      ret = 0;
    }
  }

  return ret;
}

/**
  Exchange information with the host.
*/
int32_t sdsExchange (void) {

  // Emulate that host is alive.
  sdsFlagsModify(SDS_FLAG_ALIVE, 0U);

  return SDS_OK;
}
