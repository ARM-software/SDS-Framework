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

#ifndef _FSL_IOMUXC_EX_H_
#define _FSL_IOMUXC_EX_H_

#include "fsl_common.h"
#include "fsl_iomuxc.h"


/*******************************************************************************
 * Definitions
 ******************************************************************************/
/*! @brief Internal resistor pull feature selection. */
typedef enum _iomuxc_pull
{
    kIOMUXC_PullDisable   = 0U,  /*!< Internal pull-up/down resistor is disabled. */
    kIOMUXC_PullDown_100K = 1U,  /*!< Internal pull-down 100k resistor is enabled. */
    kIOMUXC_PullUp_47K    = 2U,  /*!< Internal pull-up 47k resistor is enabled. */
    kIOMUXC_PullUp_100K   = 3U,  /*!< Internal pull-up 100k resistor is enabled. */
    kIOMUXC_PullUp_22K    = 4U,   /*!< Internal pull-up 22k resistor is enabled. */
} iomuxc_pull_t;

/*******************************************************************************
 * API
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @brief Sets IOMUXC general configuration for OpenDran setting.
 *
 * @param base     The IOMUXC configRegister address.
 * @param mode     The OpenDran setting.
 * @param enable   Pin open drain configuration.
 */
static inline void IOMUXC_EnableOpenDran(uint32_t configRegister, bool enable)
{
    *((volatile uint32_t *)configRegister) = (*((volatile uint32_t *)configRegister) & ~IOMUXC_SW_PAD_CTL_PAD_ODE_MASK) | IOMUXC_SW_PAD_CTL_PAD_ODE(enable);
}

/*!
 * @brief Sets IOMUXC general configuration for pull-up/down Config.
 *
 * @param base     The IOMUXC configRegister address.
 * @param mode     The pull-up/down Config.
 * @param value    Pin pull value
 *        - kIOMUXC_PullDisable   = 0U - Internal pull-up/down resistor is disabled.
 *        - kIOMUXC_PullDown_100K = 1U - Internal pull-down 100k resistor is enabled.
 *        - kIOMUXC_PullUp_47K    = 2U - Internal pull-up 47k resistor is enabled.
 *        - kIOMUXC_PullUp_100K   = 3U - Internal pull-up 100k resistor is enabled.
 *        - kIOMUXC_PullUp_22K    = 4U - Internal pull-up 22k resistor is enabled.
 */
static inline void IOMUXC_SetPinPullConfig(uint32_t configRegister, iomuxc_pull_t value)
{
    uint32_t tmp;

    tmp = *((volatile uint32_t *)configRegister);
    if (value == kIOMUXC_PullDisable)
    {
        tmp &= ~(IOMUXC_SW_PAD_CTL_PAD_PUS_MASK |
                 IOMUXC_SW_PAD_CTL_PAD_PKE_MASK |
                 IOMUXC_SW_PAD_CTL_PAD_PUE_MASK);
    }
    else
    {
        tmp &=  ~IOMUXC_SW_PAD_CTL_PAD_PUS_MASK;
        tmp |=   IOMUXC_SW_PAD_CTL_PAD_PKE(1U) |
                 IOMUXC_SW_PAD_CTL_PAD_PUE(1U) |
                 IOMUXC_SW_PAD_CTL_PAD_PUS(((uint32_t)value - 1U) & 0x03U);
    }
    *((volatile uint32_t *)configRegister) = tmp;
}

#if defined(__cplusplus)
}
#endif

#endif /* _FSL_IOMUXC_EX_H_ */
