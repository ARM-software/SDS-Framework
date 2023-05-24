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

// SDS I/O interface via File System (Keil::File System)

#include <string.h>
#include <stdio.h>

#include "rl_fs.h"                      // Keil.MDK-Plus::File System:CORE
#include "sdsio.h"

// File system drive
#ifndef SDSIO_DRIVE
#define SDSIO_DRIVE                 "M0:"
#endif

// Working directory:
// - If the drive prefix is omitted, the current drive is used.
// - Path string must end with delimiter character '\\' or '/'.
#ifndef SDSIO_WORK_DIR
#define SDSIO_WORK_DIR              ""
#endif

#ifndef SDSIO_MAX_NAME_SIZE
#define SDSIO_MAX_NAME_SIZE         32
#endif

// Formating of the Drive: 1 - allowed, 0 - not allowed
#ifndef SDSIO_FORMATTING_ALLOWED
#define SDSIO_FORMATTING_ALLOWED    1
#endif

// Max length of index and file extension
#define SDSIO_MAX_EXT_SIZE          16

// SDS I/O functions

/** Initialize I/O interface */
int32_t sdsioInit (void) {
  uint32_t  stat;
  int32_t   ret  = SDSIO_ERROR;

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
    ret = SDSIO_OK;
  }
  return ret;
}

/** Un-initialize I/O interface */
int32_t sdsioUninit (void) {
  funmount(SDSIO_DRIVE);
  funinit(SDSIO_DRIVE);
  return SDSIO_OK;
}

/** Open I/O stream */
sdsioId_t sdsioOpen (const char *name, sdsioMode_t mode) {
  char       file_name[sizeof(SDSIO_WORK_DIR) + SDSIO_MAX_NAME_SIZE + SDSIO_MAX_EXT_SIZE];
  uint32_t   index   = 0U;
  sdsioId_t  sdsioId = NULL;
  FILE      *file    = NULL;

  if (strlen(name) <= SDSIO_MAX_NAME_SIZE) {
    switch (mode) {
      case sdsioModeRead:
        // Not Supported
        break;
      case sdsioModeWrite:
        while (sdsioId == NULL) {
          sprintf(file_name, "%s%s.%i.sds", SDSIO_WORK_DIR, name, index);
          file = fopen(file_name, "rb");
          if (file != NULL) {
            // File already exists
            fclose(file);
            index++;
          } else {
            file = fopen(file_name, "wb");
            sdsioId = (sdsioId_t)file;
            break;
          }
        }
        break;
    }
  }
  return sdsioId;
}

/** Close I/O stream. */
int32_t sdsioClose (sdsioId_t id) {
  int32_t ret  = SDSIO_ERROR;
  FILE   *file = (FILE *)id;

  if (fclose(file) == 0) {
    ret = SDSIO_OK;
  }
  return ret;
}

/** Write data to I/O stream. */
uint32_t sdsioWrite (sdsioId_t id, const void *buf, uint32_t buf_size) {
  FILE *file = (FILE *)id;
  return fwrite(buf, 1, buf_size, file);
}

/** Read data from I/O stream. */
uint32_t sdsioRead (sdsioId_t id, void *buf, uint32_t buf_size) {
  FILE *file = (FILE *)id;
  return fread(buf, 1, buf_size, file);
}
