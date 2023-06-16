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
 * $Date:        28. March 2023
 * $Revision:    V1.0
 *
 * Project:      GPIO Driver for STM32U5xx
 */

#include "stm32u5xx_hal.h"
#include "stm32u5xx_ll_gpio.h"
#include "stm32u5xx_ll_exti.h"

#include "GPIO_STM32U5xx.h"


// Pin mapping
//   0 ..  15: PORTA 0..15
//  16 ..  31: PORTB 0..15
//  32 ..  47: PORTC 0..15
//  48 ..  63: PORTD 0..15
//  64 ..  79: PORTE 0..15
//  80 ..  95: PORTF 0..15
//  96 .. 111: PORTG 0..15
// 112 .. 127: PORTH 0..15
// 128 .. 143: PORTI 0..15
// 144 .. 159: PORTJ 0..15

#define GPIO_MAX_PORTS          10U
#define GPIO_MAX_PINS           160U


// GPIOx Peripherals
static GPIO_TypeDef * const GPIOx[GPIO_MAX_PORTS] = {
  #ifdef GPIOA
    GPIOA,
  #else
    NULL,
  #endif
  #ifdef GPIOB
    GPIOB,
  #else
    NULL,
  #endif
  #ifdef GPIOC
    GPIOC,
  #else
    NULL,
  #endif
  #ifdef GPIOD
    GPIOD,
  #else
    NULL,
  #endif
  #ifdef GPIOE
    GPIOE,
  #else
    NULL,
  #endif
  #ifdef GPIOF
    GPIOF,
  #else
    NULL,
  #endif
  #ifdef GPIOG
    GPIOG,
  #else
    NULL,
  #endif
  #ifdef GPIOH
    GPIOH,
  #else
    NULL,
  #endif
  #ifdef GPIOI
    GPIOI,
  #else
    NULL,
  #endif
  #ifdef GPIOJ
    GPIOJ
  #else
    NULL
  #endif
};


// GPIO Clock Enable
static void GPIO_ClockEnable (GPIO_TypeDef *gpio) {
#ifdef GPIOA
  if (gpio == GPIOA) __GPIOA_CLK_ENABLE();
#endif
#ifdef GPIOB
  if (gpio == GPIOB) __GPIOB_CLK_ENABLE();
#endif
#ifdef GPIOC
  if (gpio == GPIOC) __GPIOC_CLK_ENABLE();
#endif
#ifdef GPIOD
  if (gpio == GPIOD) __GPIOD_CLK_ENABLE();
#endif
#ifdef GPIOE
  if (gpio == GPIOE) __GPIOE_CLK_ENABLE();
#endif
#ifdef GPIOF
  if (gpio == GPIOF) __GPIOF_CLK_ENABLE();
#endif
#ifdef GPIOG
  if (gpio == GPIOG) __GPIOG_CLK_ENABLE();
#endif
#ifdef GPIOH
  if (gpio == GPIOH) __GPIOH_CLK_ENABLE();
#endif
#ifdef GPIOI
  if (gpio == GPIOI) __GPIOI_CLK_ENABLE();
#endif
#ifdef GPIOJ
  if (gpio == GPIOJ) __GPIOJ_CLK_ENABLE();
#endif
}


// EXTI Line Parameters
static uint32_t const EXTI_input[16] = {
  LL_EXTI_EXTI_LINE0,  LL_EXTI_EXTI_LINE1,  LL_EXTI_EXTI_LINE2,  LL_EXTI_EXTI_LINE3,
  LL_EXTI_EXTI_LINE4,  LL_EXTI_EXTI_LINE5,  LL_EXTI_EXTI_LINE6,  LL_EXTI_EXTI_LINE7,
  LL_EXTI_EXTI_LINE8,  LL_EXTI_EXTI_LINE9,  LL_EXTI_EXTI_LINE10, LL_EXTI_EXTI_LINE11,
  LL_EXTI_EXTI_LINE12, LL_EXTI_EXTI_LINE13, LL_EXTI_EXTI_LINE14, LL_EXTI_EXTI_LINE15,
};

// EXTIx IRQ Numbers
static IRQn_Type const EXTIx_IRQn[16] = {
  EXTI0_IRQn,  EXTI1_IRQn,  EXTI2_IRQn,  EXTI3_IRQn,
  EXTI4_IRQn,  EXTI5_IRQn,  EXTI6_IRQn,  EXTI7_IRQn,
  EXTI8_IRQn,  EXTI9_IRQn,  EXTI10_IRQn, EXTI11_IRQn,
  EXTI12_IRQn, EXTI13_IRQn, EXTI14_IRQn, EXTI15_IRQn
};


// Signal Event callback functions
static ARM_GPIO_SignalEvent_t SignalEvent[16] = {
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};

// Signal Ports
static uint8_t SignalPort[16] = {
  0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU,
  0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU
};


// EXTI0 IRQ Handler
void EXTI0_IRQHandler (void) {
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);
}

// EXTI1 IRQ Handler
void EXTI1_IRQHandler (void) {
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_1);
}

// EXTI2 IRQ Handler
void EXTI2_IRQHandler (void) {
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_2);
}

// EXTI3 IRQ Handler
void EXTI3_IRQHandler (void) {
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_3);
}

// EXTI4 IRQ Handler
void EXTI4_IRQHandler (void) {
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_4);
}

// EXTI5 IRQ Handler
void EXTI5_IRQHandler (void) {
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_5);
}

// EXTI6 IRQ Handler
void EXTI6_IRQHandler (void) {
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_6);
}

// EXTI7 IRQ Handler
void EXTI7_IRQHandler (void) {
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_7);
}

// EXTI8 IRQ Handler
void EXTI8_IRQHandler (void) {
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_8);
}

// EXTI9 IRQ Handler
void EXTI9_IRQHandler (void) {
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_9);
}

// EXTI10 IRQ Handler
void EXTI10_IRQHandler (void) {
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_10);
}

// EXTI11 IRQ Handler
void EXTI11_IRQHandler (void) {
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_11);
}

// EXTI12 IRQ Handler
void EXTI12_IRQHandler (void) {
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_12);
}

// EXTI13 IRQ Handler
void EXTI13_IRQHandler (void) {
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_13);
}

// EXTI14 IRQ Handler
void EXTI14_IRQHandler (void) {
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_14);
}

// EXTI15 IRQ Handler
void EXTI15_IRQHandler (void) {
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_15);
}


// EXTI line rising detection callback
void HAL_GPIO_EXTI_Rising_Callback(uint16_t GPIO_Pin) {
  uint32_t pin_num = POSITION_VAL(GPIO_Pin);

  if (SignalEvent[pin_num] != NULL) {
    SignalEvent[pin_num](((uint32_t)SignalPort[pin_num] << 4U) | pin_num, ARM_GPIO_EVENT_RISING_EDGE);
  }
}

// EXTI line falling detection callback
void HAL_GPIO_EXTI_Falling_Callback(uint16_t GPIO_Pin) {
  uint32_t pin_num = POSITION_VAL(GPIO_Pin);

  if (SignalEvent[pin_num] != NULL) {
    SignalEvent[pin_num](((uint32_t)SignalPort[pin_num] << 4U) | pin_num, ARM_GPIO_TRIGGER_FALLING_EDGE);
  }
}


// Setup GPIO Interface
static int32_t GPIO_Setup (ARM_GPIO_Pin_t pin, ARM_GPIO_SignalEvent_t cb_event) {
  GPIO_TypeDef    *gpio;
  GPIO_InitTypeDef init;
  uint32_t         pin_port;
  uint32_t         pin_num;
  uint32_t         pin_mask;
  int32_t          result = ARM_DRIVER_OK;

  if (pin < GPIO_MAX_PINS) {
    pin_port = pin >> 4U;
    pin_num  = pin & 0x0FU;
    pin_mask = 1U << pin_num;
    gpio = GPIOx[pin_port];
    if (gpio != NULL) {
      if ((cb_event == NULL) || (SignalPort[pin_num] == 0xFFU) || (SignalPort[pin_num] == pin_port)) {
        init.Pin       = pin_mask;
        init.Mode      = GPIO_MODE_INPUT;
        init.Pull      = GPIO_NOPULL;
        init.Speed     = GPIO_SPEED_FREQ_HIGH;
        init.Alternate = 0U;
        GPIO_ClockEnable(gpio);
        if ((SignalPort[pin_num] == 0xFFU) || (SignalPort[pin_num] == pin_port)) {
          LL_EXTI_DisableIT_0_31(pin_mask);
        }
        HAL_GPIO_WritePin(gpio, (uint16_t)pin_mask, GPIO_PIN_RESET);
        HAL_GPIO_Init(gpio, &init);
        if (cb_event != NULL) {
          SignalEvent[pin_num] = cb_event;
          SignalPort [pin_num] = (uint8_t)pin_port;
          NVIC_EnableIRQ(EXTIx_IRQn[pin_num]);
        }
      } else {
        result = ARM_DRIVER_ERROR;
      }
    } else {
      result = ARM_GPIO_ERROR_PIN;
    }
  } else {
    result = ARM_GPIO_ERROR_PIN;
  }

  return result;
}

// Set GPIO Direction
static int32_t GPIO_SetDirection (ARM_GPIO_Pin_t pin, ARM_GPIO_DIRECTION direction) {
  GPIO_TypeDef *gpio;
  uint32_t      pin_port;
  uint32_t      pin_mask;
  int32_t       result = ARM_DRIVER_OK;

  if (pin < GPIO_MAX_PINS) {
    pin_port = pin >> 4U;
    pin_mask = 1U << (pin & 0x0FU);
    gpio = GPIOx[pin_port];
    if (gpio != NULL) {
      switch (direction) {
        case ARM_GPIO_INPUT:
          LL_GPIO_SetPinMode(gpio, pin_mask, LL_GPIO_MODE_INPUT);
          break;
        case ARM_GPIO_OUTPUT:
          LL_GPIO_SetPinMode(gpio, pin_mask, LL_GPIO_MODE_OUTPUT);
          break;
        default:
          result = ARM_DRIVER_ERROR_PARAMETER;
          break;
      }
    } else {
      result = ARM_GPIO_ERROR_PIN;
    }
  } else {
    result = ARM_GPIO_ERROR_PIN;
  }

  return result;
}

// Set GPIO Output Mode
static int32_t GPIO_SetOutputMode (ARM_GPIO_Pin_t pin, ARM_GPIO_OUTPUT_MODE mode) {
  GPIO_TypeDef *gpio;
  uint32_t      pin_port;
  uint32_t      pin_mask;
  int32_t       result = ARM_DRIVER_OK;

  if (pin < GPIO_MAX_PINS) {
    pin_port = pin >> 4U;
    pin_mask = 1U << (pin & 0x0FU);
    gpio = GPIOx[pin_port];
    if (gpio != NULL) {
      switch (mode) {
        case ARM_GPIO_PUSH_PULL:
          LL_GPIO_SetPinOutputType(gpio, pin_mask, LL_GPIO_OUTPUT_PUSHPULL);
          break;
        case ARM_GPIO_OPEN_DRAIN:
          LL_GPIO_SetPinOutputType(gpio, pin_mask, LL_GPIO_OUTPUT_OPENDRAIN);
          break;
        default:
          result = ARM_DRIVER_ERROR_PARAMETER;
          break;
      }
    } else {
      result = ARM_GPIO_ERROR_PIN;
    }
  } else {
    result = ARM_GPIO_ERROR_PIN;
  }

  return result;
}

// Set GPIO Pull Resistor
static int32_t GPIO_SetPullResistor (ARM_GPIO_Pin_t pin, ARM_GPIO_PULL_RESISTOR resistor) {
  GPIO_TypeDef *gpio;
  uint32_t      pin_port;
  uint32_t      pin_mask;
  int32_t       result = ARM_DRIVER_OK;

  if (pin < GPIO_MAX_PINS) {
    pin_port = pin >> 4U;
    pin_mask = 1U << (pin & 0x0FU);
    gpio = GPIOx[pin_port];
    if (gpio != NULL) {
      switch (resistor) {
        case ARM_GPIO_PULL_NONE:
          LL_GPIO_SetPinPull(gpio, pin_mask, LL_GPIO_PULL_NO);
          break;
        case ARM_GPIO_PULL_UP:
          LL_GPIO_SetPinPull(gpio, pin_mask, LL_GPIO_PULL_UP);
          break;
        case ARM_GPIO_PULL_DOWN:
          LL_GPIO_SetPinPull(gpio, pin_mask, LL_GPIO_PULL_DOWN);
          break;
        default:
          result = ARM_DRIVER_ERROR_PARAMETER;
          break;
      }
    } else {
      result = ARM_GPIO_ERROR_PIN;
    }
  } else {
    result = ARM_GPIO_ERROR_PIN;
  }

  return result;
}

// Set GPIO Event Trigger
static int32_t GPIO_SetEventTrigger (ARM_GPIO_Pin_t pin, ARM_GPIO_EVENT_TRIGGER trigger) {
  GPIO_TypeDef *gpio;
  uint32_t      pin_port;
  uint32_t      pin_num;
  uint32_t      pin_mask;
  int32_t       result = ARM_DRIVER_OK;

  if (pin < GPIO_MAX_PINS) {
    pin_port = pin >> 4U;
    pin_num  = pin & 0x0FU;
    pin_mask = 1U << pin_num;
    gpio = GPIOx[pin_port];
    if (gpio != NULL) {
      switch (trigger) {
        case ARM_GPIO_TRIGGER_NONE:
          if (SignalPort[pin_num] == (uint8_t)pin_port) {
            LL_EXTI_DisableIT_0_31(pin_mask);
          }
          break;
        case ARM_GPIO_TRIGGER_RISING_EDGE:
          if (SignalPort[pin_num] == (uint8_t)pin_port) {
            LL_EXTI_SetEXTISource(pin_port, EXTI_input[pin_num]);
            LL_EXTI_DisableFallingTrig_0_31(pin_mask);
            LL_EXTI_EnableRisingTrig_0_31(pin_mask);
            LL_EXTI_EnableIT_0_31(pin_mask);
          }
          break;
        case ARM_GPIO_TRIGGER_FALLING_EDGE:
          if (SignalPort[pin_num] == (uint8_t)pin_port) {
            LL_EXTI_SetEXTISource(pin_port, EXTI_input[pin_num]);
            LL_EXTI_DisableRisingTrig_0_31(pin_mask);
            LL_EXTI_EnableFallingTrig_0_31(pin_mask);
            LL_EXTI_EnableIT_0_31(pin_mask);
          }
          break;
        case ARM_GPIO_TRIGGER_EITHER_EDGE:
          if (SignalPort[pin_num] == (uint8_t)pin_port) {
            LL_EXTI_SetEXTISource(pin_port, EXTI_input[pin_num]);
            LL_EXTI_EnableRisingTrig_0_31(pin_mask);
            LL_EXTI_EnableFallingTrig_0_31(pin_mask);
            LL_EXTI_EnableIT_0_31(pin_mask);
          }
          break;
        default:
          result = ARM_DRIVER_ERROR_PARAMETER;
          break;
      }
    } else {
      result = ARM_GPIO_ERROR_PIN;
    }
  } else {
    result = ARM_GPIO_ERROR_PIN;
  }

  return result;
}

// Set GPIO Output Level
static void GPIO_SetOutput (ARM_GPIO_Pin_t pin, uint32_t val) {
  GPIO_TypeDef *gpio;
  uint32_t      pin_port;
  uint32_t      pin_mask;

  if (pin < GPIO_MAX_PINS) {
    pin_port = pin >> 4U;
    pin_mask = 1U << (pin & 0x0FU);
    gpio = GPIOx[pin_port];
    if (gpio != NULL) {
      HAL_GPIO_WritePin(gpio, (uint16_t)pin_mask, (val != 0U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }
  }
}

// Get GPIO Input Level
static uint32_t GPIO_GetInput (ARM_GPIO_Pin_t pin) {
  GPIO_TypeDef *gpio;
  GPIO_PinState pin_state;
  uint32_t      pin_port;
  uint32_t      pin_mask;
  uint32_t      val = 0U;

  if (pin < GPIO_MAX_PINS) {
    pin_port = pin >> 4U;
    pin_mask = 1U << (pin & 0x0FU);
    gpio = GPIOx[pin_port];
    if (gpio != NULL) {
      pin_state = HAL_GPIO_ReadPin(gpio, (uint16_t)pin_mask);
      if (pin_state == GPIO_PIN_SET) {
        val = 1U;
      }
    }
  }
  return val;
}


// GPIO Driver access structure
ARM_DRIVER_GPIO Driver_GPIO0 = {
  GPIO_Setup,
  GPIO_SetDirection,
  GPIO_SetOutputMode,
  GPIO_SetPullResistor,
  GPIO_SetEventTrigger,
  GPIO_SetOutput,
  GPIO_GetInput
};
