

# File sdsio\_client.h

[**File List**](files.md) **>** [**include**](dir_d09908635ef304ba819d3349bcb716bf.md) **>** [**sdsio\_client.h**](sdsio__client_8h.md)

[Go to the documentation of this file](sdsio__client_8h.md)


```C++
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

#ifndef SDSIO_CLIENT_H
#define SDSIO_CLIENT_H

#ifdef  __cplusplus
 extern "C"
{
#endif

// SDSIO Client works in a pair with SDSIO Server. Communication protocol is documented in the following link:
// https://arm-software.github.io/SDS-Framework/main/theory/#sdsio-server-protocol

 // SDSIO header
typedef struct {
  uint32_t command;
  uint32_t sdsio_id;
  uint32_t argument;
  uint32_t data_size;
} header_t;

// SDSIO Server Command IDs
#define SDSIO_CMD_OPEN          1U
#define SDSIO_CMD_CLOSE         2U
#define SDSIO_CMD_WRITE         3U
#define SDSIO_CMD_READ          4U
#define SDSIO_CMD_PING          5U

// Function prototypes

int32_t sdsioClientInit (void);

int32_t sdsioClientUninit (void);

int32_t sdsioClientSend (const uint8_t *buf, uint32_t buf_size);

int32_t sdsioClientReceive (uint8_t *buf, uint32_t buf_size);

#ifdef  __cplusplus
}
#endif

#endif  /* SDSIO_CLIENT_H */
```


