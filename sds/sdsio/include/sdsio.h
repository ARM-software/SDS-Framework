/*
 * Copyright (c) 2022-2026 Arm Limited. All rights reserved.
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

#ifndef SDSIO_H
#define SDSIO_H

#ifdef  __cplusplus
extern "C"
{
#endif

#include <stdint.h>

// ==== Synchronous Data Stream Input/Output (SDSIO) ====

typedef void *sdsioId_t;        // Handle to SDSIO stream

// Open Mode
typedef enum {
  sdsioModeRead  = 0,           // Open for read (binary)
  sdsioModeWrite = 1            // Open for write (binary)
} sdsioMode_t;                  // Open mode (read/write)

/**
  \fn          int32_t sdsioInit (void)
  \brief       Initialize SDSIO interface.
  \return      SDS_OK on success or
               a negative value on error (see \ref SDS_Return_Codes)
*/
int32_t sdsioInit (void);

/**
  \fn          int32_t sdsioUninit (void)
  \brief       Un-initialize SDSIO interface.
  \return      SDS_OK on success or
               a negative value on error (see \ref SDS_Return_Codes)
*/
int32_t sdsioUninit (void);

/**
  \fn          sdsioId_t sdsioOpen (const char *name, sdsioMode_t mode)
  \brief       Open SDSIO stream.
  \param[in]   name           stream name (pointer to NULL terminated string)
  \param[in]   mode           \ref sdsioMode_t open mode
  \return      \ref sdsioId_t Handle to SDSIO stream, or NULL if operation failed
*/
sdsioId_t sdsioOpen (const char *name, sdsioMode_t mode);

/**
  \fn          int32_t sdsioClose (sdsioId_t id)
  \brief       Close SDSIO stream.
  \param[in]   id             \ref sdsioId_t handle to SDSIO stream
  \return      SDS_OK on success or
               a negative value on error (see \ref SDS_Return_Codes)
*/
int32_t sdsioClose (sdsioId_t id);

/**
  \fn          int32_t sdsioWrite (sdsioId_t id, const void *buf, uint32_t buf_size)
  \brief       Write data to SDSIO stream.
  \param[in]   id             \ref sdsioId_t handle to SDSIO stream
  \param[in]   buf            pointer to buffer with data to write
  \param[in]   buf_size       buffer size in bytes
  \return      number of bytes successfully written or
               a negative value on error (see \ref SDS_Return_Codes)
*/
int32_t sdsioWrite (sdsioId_t id, const void *buf, uint32_t buf_size);

/**
  \fn          int32_t sdsioRead (sdsioId_t id, void *buf, uint32_t buf_size)
  \brief       Read data from SDSIO stream.
  \param[in]   id             \ref sdsioId_t handle to SDSIO stream
  \param[out]  buf            pointer to buffer for data to read
  \param[in]   buf_size       buffer size in bytes
  \return      number of bytes successfully read, or
               a negative value on error or SDS_EOS (see \ref SDS_Return_Codes)
*/
int32_t sdsioRead (sdsioId_t id, void *buf, uint32_t buf_size);

#ifdef  __cplusplus
}
#endif

#endif  /* SDSIO_H */
