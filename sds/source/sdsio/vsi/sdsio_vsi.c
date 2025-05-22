/*
 * Copyright (c) 2023-2025 Arm Limited. All rights reserved.
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

// SDS I/O interface via VSI

#include <stddef.h>

#include "sdsio.h"


// SDS I/O functions

/** Initialize I/O interface */
int32_t sdsioInit (void) {
  return SDSIO_OK;
}

/** Un-initialize I/O interface */
int32_t sdsioUninit (void) {
  return SDSIO_OK;
}

/** Open I/O stream */
sdsioId_t sdsioOpen (const char *name, sdsioMode_t mode) {
  return NULL;
}

/** Close I/O stream. */
int32_t sdsioClose (sdsioId_t id) {
  return SDSIO_OK;
}

/** Write data to I/O stream. */
int32_t sdsioWrite (sdsioId_t id, const void *buf, uint32_t buf_size) {
  return SDSIO_ERROR;
}

/** Read data from I/O stream. */
int32_t sdsioRead (sdsioId_t id, void *buf, uint32_t buf_size) {
  return SDSIO_ERROR;
}
