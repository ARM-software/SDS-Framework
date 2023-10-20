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

#ifndef SDSIO_H
#define SDSIO_H

#ifdef  __cplusplus
extern "C"
{
#endif

#include <stdint.h>

// ==== Synchronous Data Stream Input/Output (SDS I/O) ====

/// Identifier
typedef void *sdsioId_t;

/// Open Mode
typedef enum {
  sdsioModeRead  = 0,           ///< Open for read (binary)
  sdsioModeWrite = 1            ///< Open for write (binary)
} sdsioMode_t;

/// Function return codes
#define SDSIO_OK                (0)         ///< Operation completed successfully
#define SDSIO_ERROR             (-1)        ///< Operation failed

/**
  \fn          int32_t sdsioInit (void)
  \brief       Initialize SDS I/O.
  \return      return code
*/
int32_t sdsioInit (void);

/**
  \fn          int32_t sdsioUninit (void)
  \brief       Un-initialize SDS I/O.
  \return      return code
*/
int32_t sdsioUninit (void);

/**
  \fn          sdsioId_t sdsioOpen (const char *name, sdsioMode_t mode)
  \brief       Open I/O stream.
  \param[in]   name           stream name (pointer to NULL terminated string)
  \param[in]   mode           \ref sdsioMode_t
  \return      \ref sdsioId_t
*/
sdsioId_t sdsioOpen (const char *name, sdsioMode_t mode);

/**
  \fn          int32_t sdsioClose (sdsioId_t id)
  \brief       Close I/O stream.
  \param[in]   id             \ref sdsioId_t
  \return      return code
*/
int32_t sdsioClose (sdsioId_t id);

/**
  \fn          uint32_t sdsioWrite (sdsioId_t id, const void *buf, uint32_t buf_size)
  \brief       Write data to I/O stream.
  \param[in]   id             \ref sdsioId_t
  \param[in]   buf            pointer to buffer with data to write
  \param[in]   buf_size       buffer size in bytes
  \return      number of bytes written
*/
uint32_t sdsioWrite (sdsioId_t id, const void *buf, uint32_t buf_size);

/**
  \fn          uint32_t sdsioRead (sdsioId_t id, void *buf, uint32_t buf_size)
  \brief       Read data from I/O stream.
  \param[in]   id             \ref sdsioId_t
  \param[out]  buf            pointer to buffer for data to read
  \param[in]   buf_size       buffer size in bytes
  \return      number of bytes read
*/
uint32_t sdsioRead (sdsioId_t id, void *buf, uint32_t buf_size);

/**
  \fn          int32_t sdsioEndOfStream (sdsioId_t id)
  \brief       Check if end of stream has been reached.
  \param[in]   id             \ref sdsioId_t
  \return      nonzero if end of stream, else 0
*/
int32_t sdsioEndOfStream (sdsioId_t id);

#ifdef  __cplusplus
}
#endif

#endif  /* SDSIO_H */
