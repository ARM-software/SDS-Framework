/*
 * Copyright (c) 2023 ARM Limited. All rights reserved.
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
 *
 * $Date:        10. February 2023
 * $Revision:    V1.0
 *
 * Project:      GPIO Driver definitions for i.MX RT1050
 */

#ifndef GPIO_IMXRT1050_H_
#define GPIO_IMXRT1050_H_

#ifdef  __cplusplus
extern "C"
{
#endif

#include "Driver_GPIO.h"

// Pin mapping
#define GPIO_PORT1(n)   (  0U + (n))
#define GPIO_PORT2(n)   ( 32U + (n))
#define GPIO_PORT3(n)   ( 64U + (n))
#define GPIO_PORT4(n)   ( 96U + (n))
#define GPIO_PORT5(n)   (128U + (n))

// PORTx IRQ Handlers
extern void GPIO1_Combined_0_15_IRQHandler (void);
extern void GPIO1_Combined_16_31_IRQHandler (void);
extern void GPIO2_Combined_0_15_IRQHandler (void);
extern void GPIO2_Combined_16_31_IRQHandler (void);
extern void GPIO3_Combined_0_15_IRQHandler (void);
extern void GPIO3_Combined_16_31_IRQHandler (void);
extern void GPIO4_Combined_0_15_IRQHandler (void);
extern void GPIO4_Combined_16_31_IRQHandler (void);
extern void GPIO5_Combined_0_15_IRQHandler (void);
extern void GPIO5_Combined_16_31_IRQHandler (void);

// GPIO Driver access structure
extern ARM_DRIVER_GPIO Driver_GPIO0;

#ifdef  __cplusplus
}
#endif

#endif /* GPIO_IMXRT1050_H_ */
