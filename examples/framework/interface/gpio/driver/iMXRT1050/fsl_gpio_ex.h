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
 */

#ifndef _FSL_GPIO_EX_H_
#define _FSL_GPIO_EX_H_

#include "fsl_common.h"
#include "fsl_gpio.h"


/*******************************************************************************
 * Definitions
 ******************************************************************************/


/*******************************************************************************
 * API
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * brief Gets the current pin interrupt configiration.
 *
 * @param base   GPIO base pointer.
 * @param pin    GPIO port pin number.
 * @return current pin interrupt configiration from ICRx register value.
 */
static inline gpio_interrupt_mode_t GPIO_GetPinConfig(GPIO_Type *base, uint32_t pin)
{
    uint32_t icr;

    if ((base->EDGE_SEL & (0x1U << pin )) >> pin)
    {
        return kGPIO_IntRisingOrFallingEdge;
    }

    if (pin < 16U)
    {    //     register     pin mask                 pin shift
         icr = (base->ICR1 & (0x03U << (pin << 1U))) >> (pin << 1U);
    }
    else
    {   //     register      pin mask                 pin shift
         icr = (base->ICR2 & (0x03U << (pin << 1U))) >> (pin << 1U);
    }

    return (gpio_interrupt_mode_t)(icr + 1U);
}

/*!
 * @brief Sets the direction of GPIO pin.
 *
 * @param base      GPIO peripheral base pointer (GPIO1, GPIO2, GPIO3, and so on.)
 * @param pin       GPIO pin number
 * @param direction GPIO pin direction.
 *        - kGPIO_DigitalInput  = 0U - Set current pin as digital input
 *        - kGPIO_DigitalOutput = 1U - Set current pin as digital output
 */
static inline void GPIO_PinSetDirection(GPIO_Type *base, uint32_t pin, gpio_pin_direction_t direction)
{
    if (direction == kGPIO_DigitalOutput)
    {
        base->GDIR |=  (1UL << pin);
    }
    else
    {
        base->GDIR &= ~(1UL << pin);
    }
}

#if defined(__cplusplus)
}
#endif


#endif /* _FSL_GPIO_EX_H_*/
