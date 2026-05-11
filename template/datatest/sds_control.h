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

#ifndef SDS_CONTROL_H_
#define SDS_CONTROL_H_

#include <stdint.h>
#include "cmsis_compiler.h"

#ifdef  __cplusplus
extern "C"
{
#endif

// SDS control thread function
extern __NO_RETURN void sdsControlThread (void *argument);

// Application main function
extern int32_t app_main (void);

#ifdef  __cplusplus
}
#endif

#endif
