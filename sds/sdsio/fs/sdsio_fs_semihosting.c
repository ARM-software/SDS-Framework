/*
 * Copyright (c) 2023, 2026 Arm Limited. All rights reserved.
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

// SDS I/O interface via File System (Semihosting)

#include <string.h>
#include <stdio.h>

#include "sds.h"
#include "sdsio.h"


// Maximum stream name length
#ifndef SDSIO_MAX_NAME_SIZE
#define SDSIO_MAX_NAME_SIZE         32U
#endif

// Max length of index and file extension
#define SDSIO_MAX_EXT_SIZE          16U

// SDS I/O functions

/**
  \fn          int32_t sdsioInit (void)
  \brief       Initialize SDS I/O interface.
  \return      SDS_OK on success or
               a negative value on error (see \ref SDS_Return_Codes)
*/
int32_t sdsioInit (void) {
  SDS_PRINTF("SDS I/O File System (SemiHosting) interface initialized successfully\n");
  return SDS_OK;
}

/**
  \fn          int32_t sdsioUninit (void)
  \brief       Un-initialize SDS I/O interface.
  \return      SDS_OK on success or
               a negative value on error (see \ref SDS_Return_Codes)
*/
int32_t sdsioUninit (void) {
  return SDS_OK;
}

/**
  \fn          sdsioId_t sdsioOpen (const char *name, sdsioMode_t mode)
  \brief       Open I/O stream.
  \param[in]   name           stream name (pointer to NULL terminated string)
  \param[in]   mode           \ref sdsioMode_t open mode
  \return      \ref sdsioId_t Handle to SDS I/O stream, or NULL if operation failed
*/
sdsioId_t sdsioOpen (const char *name, sdsioMode_t mode) {
  uint32_t   index   = 0U;
  sdsioId_t  sdsioId = NULL;
  FILE      *file    = NULL;
  char       file_name[SDSIO_MAX_NAME_SIZE + SDSIO_MAX_EXT_SIZE + 1];
  char       line[16];

  if (strlen(name) <= SDSIO_MAX_NAME_SIZE) {
    switch (mode) {
      case sdsioModeRead:

        sprintf(file_name, "%s.index.txt", name);
        file = fopen(file_name, "r");
        if (file != NULL) {
          if (fscanf(file, "%i", &index) != 1) {
            index = 0U;
          }
          fclose(file);
        }

        sprintf(file_name, "%s.%i.sds", name, index);
        file = fopen(file_name, "rb");
        if (file != NULL) {
          // File exists
          index++;
          sdsioId = (sdsioId_t)file;
        } else {
          if (index != 0U) {
            index = 0U;
          }
        }

        if ((sdsioId != NULL) || (index == 0U)) {
          sprintf(file_name, "%s.index.txt", name);
          file = fopen(file_name, "w");
          if (file != NULL) {
            sprintf(line, "%i\r\n", index);
            fwrite(line, 1, strlen(line) + 1U, file);
            fclose(file);
          }
        }
        break;
      case sdsioModeWrite:
        while (sdsioId == NULL) {
          sprintf(file_name, "%s.%i.sds", name, index);
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

/**
  \fn          int32_t sdsioClose (sdsioId_t id)
  \brief       Close I/O stream.
  \param[in]   id             \ref sdsioId_t handle to SDS I/O stream
  \return      SDS_OK on success or
               a negative value on error (see \ref SDS_Return_Codes)
*/
int32_t sdsioClose (sdsioId_t id) {
  int32_t ret  = SDS_ERROR_IO;
  FILE   *file = (FILE *)id;

  if (fclose(file) == 0) {
    ret = SDS_OK;
  }
  return ret;
}

/**
  \fn          int32_t sdsioWrite (sdsioId_t id, const void *buf, uint32_t buf_size)
  \brief       Write data to I/O stream.
  \param[in]   id             \ref sdsioId_t handle to SDS I/O stream
  \param[in]   buf            pointer to buffer with data to write
  \param[in]   buf_size       buffer size in bytes
  \return      number of bytes successfully written or
               a negative value on error (see \ref SDS_Return_Codes)
*/
int32_t sdsioWrite (sdsioId_t id, const void *buf, uint32_t buf_size) {
  FILE    *file  = (FILE *)id;
  int32_t  ret   = SDS_ERROR_IO;
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
  \brief       Read data from I/O stream.
  \param[in]   id             \ref sdsioId_t handle to SDS I/O stream
  \param[out]  buf            pointer to buffer for data to read
  \param[in]   buf_size       buffer size in bytes
  \return      number of bytes successfully read, or
               a negative value on error or SDS_EOS (see \ref SDS_Return_Codes)
*/
int32_t sdsioRead (sdsioId_t id, void *buf, uint32_t buf_size) {
  FILE *file  = (FILE *)id;
  int32_t ret = SDS_ERROR_IO;
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
  This function cannot be implemented in system using SDS I/O interface via file system.
*/
int32_t sdsExchange (void) {
  return SDS_ERROR_IO;
}

/**
  This function cannot be implemented in system using SDS I/O interface via file system.
*/
void sdsFlagsModify (uint32_t set_mask, uint32_t clear_mask) {
}
