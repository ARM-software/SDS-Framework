/*---------------------------------------------------------------------------
 * Copyright (c) 2020-2021 Arm Limited (or its affiliates).
 * All rights reserved.
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
 *---------------------------------------------------------------------------*/

#ifndef MAIN_H__
#define MAIN_H__

#include <stdint.h>

/* Prototypes */
extern int32_t app_initialize (void);
extern void    app_main (void *argument);

extern int32_t VSI2_Initialize (void);
extern int32_t VSI5_Initialize (void);
extern int32_t VSI6_Initialize (void);

#endif
