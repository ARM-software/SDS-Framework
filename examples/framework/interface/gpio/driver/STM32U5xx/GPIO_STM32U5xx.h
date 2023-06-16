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
 * Project:      GPIO Driver definitions for STM32U5xx
 */

#ifndef GPIO_STM32U5XX_H_
#define GPIO_STM32U5XX_H_

#ifdef  __cplusplus
extern "C"
{
#endif

#include "Driver_GPIO.h"

// Pin mapping
#define GPIO_PORTA(n)   (  0U + (n))
#define GPIO_PORTB(n)   ( 16U + (n))
#define GPIO_PORTC(n)   ( 32U + (n))
#define GPIO_PORTD(n)   ( 48U + (n))
#define GPIO_PORTE(n)   ( 64U + (n))
#define GPIO_PORTF(n)   ( 80U + (n))
#define GPIO_PORTG(n)   ( 96U + (n))
#define GPIO_PORTH(n)   (112U + (n))
#define GPIO_PORTI(n)   (128U + (n))
#define GPIO_PORTJ(n)   (144U + (n))

// EXTIx IRQ Handlers
extern void EXTI0_IRQHandler (void);
extern void EXTI1_IRQHandler (void);
extern void EXTI2_IRQHandler (void);
extern void EXTI3_IRQHandler (void);
extern void EXTI4_IRQHandler (void);
extern void EXTI5_IRQHandler (void);
extern void EXTI6_IRQHandler (void);
extern void EXTI7_IRQHandler (void);
extern void EXTI8_IRQHandler (void);
extern void EXTI9_IRQHandler (void);
extern void EXTI10_IRQHandler (void);
extern void EXTI11_IRQHandler (void);
extern void EXTI12_IRQHandler (void);
extern void EXTI13_IRQHandler (void);
extern void EXTI14_IRQHandler (void);
extern void EXTI15_IRQHandler (void);

// GPIO0 Driver access structure
extern ARM_DRIVER_GPIO Driver_GPIO0;

#ifdef  __cplusplus
}
#endif

#endif /* GPIO_STM32U5XX_H_ */
