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
 * $Date:        21. March 2023
 * $Revision:    V1.0
 *
 * Project:      GPIO Driver for i.MX RT1050
 */

#include "fsl_clock.h"
#include "fsl_gpio.h"
#include "fsl_gpio_ex.h"
#include "fsl_iomuxc.h"
#include "fsl_iomuxc_ex.h"

#include "GPIO_iMXRT1050.h"


// Pin mapping
//   0 ..  31: GPIO1 0..31
//  32 ..  63: GPIO2 0..31
//  64 ..  95: GPIO3 0..31
//  96 .. 127: GPIO4 0..31
// 128 .. 159: GPIO5 0..31

#define GPIO_MAX_PORTS          5U
#define GPIO_MAX_PINS           160U

static const uint32_t PinMapping[GPIO_MAX_PORTS] = {
  0xFFFFFFFFU,  // GPIO1  0..31
  0xFFFFFFFFU,  // GPIO2  0..31
  0x0FFFFFFFU,  // GPIO3  0..27
  0xFFFFFFFFU,  // GPIO4  0..31
  0x00000007U   // GPIO5  0..2
};

// Pin configuration structure
typedef struct {
  uint32_t muxRegister;
  uint32_t muxMode;
  uint32_t inputRegister;
  uint32_t inputDaisy;
  uint32_t configRegister;
} pinConfig_t;

// Pin configuration
static pinConfig_t const PinConfig[] = {
  { IOMUXC_GPIO_AD_B0_00_GPIO1_IO00 },
  { IOMUXC_GPIO_AD_B0_01_GPIO1_IO01 },
  { IOMUXC_GPIO_AD_B0_02_GPIO1_IO02 },
  { IOMUXC_GPIO_AD_B0_03_GPIO1_IO03 },
  { IOMUXC_GPIO_AD_B0_04_GPIO1_IO04 },
  { IOMUXC_GPIO_AD_B0_05_GPIO1_IO05 },
  { IOMUXC_GPIO_AD_B0_06_GPIO1_IO06 },
  { IOMUXC_GPIO_AD_B0_07_GPIO1_IO07 },
  { IOMUXC_GPIO_AD_B0_08_GPIO1_IO08 },
  { IOMUXC_GPIO_AD_B0_09_GPIO1_IO09 },
  { IOMUXC_GPIO_AD_B0_10_GPIO1_IO10 },
  { IOMUXC_GPIO_AD_B0_11_GPIO1_IO11 },
  { IOMUXC_GPIO_AD_B0_12_GPIO1_IO12 },
  { IOMUXC_GPIO_AD_B0_13_GPIO1_IO13 },
  { IOMUXC_GPIO_AD_B0_14_GPIO1_IO14 },
  { IOMUXC_GPIO_AD_B0_15_GPIO1_IO15 },
  { IOMUXC_GPIO_AD_B1_00_GPIO1_IO16 },
  { IOMUXC_GPIO_AD_B1_01_GPIO1_IO17 },
  { IOMUXC_GPIO_AD_B1_02_GPIO1_IO18 },
  { IOMUXC_GPIO_AD_B1_03_GPIO1_IO19 },
  { IOMUXC_GPIO_AD_B1_04_GPIO1_IO20 },
  { IOMUXC_GPIO_AD_B1_05_GPIO1_IO21 },
  { IOMUXC_GPIO_AD_B1_06_GPIO1_IO22 },
  { IOMUXC_GPIO_AD_B1_07_GPIO1_IO23 },
  { IOMUXC_GPIO_AD_B1_08_GPIO1_IO24 },
  { IOMUXC_GPIO_AD_B1_09_GPIO1_IO25 },
  { IOMUXC_GPIO_AD_B1_10_GPIO1_IO26 },
  { IOMUXC_GPIO_AD_B1_11_GPIO1_IO27 },
  { IOMUXC_GPIO_AD_B1_12_GPIO1_IO28 },
  { IOMUXC_GPIO_AD_B1_13_GPIO1_IO29 },
  { IOMUXC_GPIO_AD_B1_14_GPIO1_IO30 },
  { IOMUXC_GPIO_AD_B1_15_GPIO1_IO31 },
  { IOMUXC_GPIO_B0_00_GPIO2_IO00 },
  { IOMUXC_GPIO_B0_01_GPIO2_IO01 },
  { IOMUXC_GPIO_B0_02_GPIO2_IO02 },
  { IOMUXC_GPIO_B0_03_GPIO2_IO03 },
  { IOMUXC_GPIO_B0_04_GPIO2_IO04 },
  { IOMUXC_GPIO_B0_05_GPIO2_IO05 },
  { IOMUXC_GPIO_B0_06_GPIO2_IO06 },
  { IOMUXC_GPIO_B0_07_GPIO2_IO07 },
  { IOMUXC_GPIO_B0_08_GPIO2_IO08 },
  { IOMUXC_GPIO_B0_09_GPIO2_IO09 },
  { IOMUXC_GPIO_B0_10_GPIO2_IO10 },
  { IOMUXC_GPIO_B0_11_GPIO2_IO11 },
  { IOMUXC_GPIO_B0_12_GPIO2_IO12 },
  { IOMUXC_GPIO_B0_13_GPIO2_IO13 },
  { IOMUXC_GPIO_B0_14_GPIO2_IO14 },
  { IOMUXC_GPIO_B0_15_GPIO2_IO15 },
  { IOMUXC_GPIO_B1_00_GPIO2_IO16 },
  { IOMUXC_GPIO_B1_01_GPIO2_IO17 },
  { IOMUXC_GPIO_B1_02_GPIO2_IO18 },
  { IOMUXC_GPIO_B1_03_GPIO2_IO19 },
  { IOMUXC_GPIO_B1_04_GPIO2_IO20 },
  { IOMUXC_GPIO_B1_05_GPIO2_IO21 },
  { IOMUXC_GPIO_B1_06_GPIO2_IO22 },
  { IOMUXC_GPIO_B1_07_GPIO2_IO23 },
  { IOMUXC_GPIO_B1_08_GPIO2_IO24 },
  { IOMUXC_GPIO_B1_09_GPIO2_IO25 },
  { IOMUXC_GPIO_B1_10_GPIO2_IO26 },
  { IOMUXC_GPIO_B1_11_GPIO2_IO27 },
  { IOMUXC_GPIO_B1_12_GPIO2_IO28 },
  { IOMUXC_GPIO_B1_13_GPIO2_IO29 },
  { IOMUXC_GPIO_B1_14_GPIO2_IO30 },
  { IOMUXC_GPIO_B1_15_GPIO2_IO31 },
  { IOMUXC_GPIO_SD_B1_00_GPIO3_IO00 },
  { IOMUXC_GPIO_SD_B1_01_GPIO3_IO01 },
  { IOMUXC_GPIO_SD_B1_02_GPIO3_IO02 },
  { IOMUXC_GPIO_SD_B1_03_GPIO3_IO03 },
  { IOMUXC_GPIO_SD_B1_04_GPIO3_IO04 },
  { IOMUXC_GPIO_SD_B1_05_GPIO3_IO05 },
  { IOMUXC_GPIO_SD_B1_06_GPIO3_IO06 },
  { IOMUXC_GPIO_SD_B1_07_GPIO3_IO07 },
  { IOMUXC_GPIO_SD_B1_08_GPIO3_IO08 },
  { IOMUXC_GPIO_SD_B1_09_GPIO3_IO09 },
  { IOMUXC_GPIO_SD_B1_10_GPIO3_IO10 },
  { IOMUXC_GPIO_SD_B1_11_GPIO3_IO11 },
  { IOMUXC_GPIO_SD_B0_00_GPIO3_IO12 },
  { IOMUXC_GPIO_SD_B0_01_GPIO3_IO13 },
  { IOMUXC_GPIO_SD_B0_02_GPIO3_IO14 },
  { IOMUXC_GPIO_SD_B0_03_GPIO3_IO15 },
  { IOMUXC_GPIO_SD_B0_04_GPIO3_IO16 },
  { IOMUXC_GPIO_SD_B0_05_GPIO3_IO17 },
  { IOMUXC_GPIO_EMC_32_GPIO3_IO18 },
  { IOMUXC_GPIO_EMC_33_GPIO3_IO19 },
  { IOMUXC_GPIO_EMC_34_GPIO3_IO20 },
  { IOMUXC_GPIO_EMC_35_GPIO3_IO21 },
  { IOMUXC_GPIO_EMC_36_GPIO3_IO22 },
  { IOMUXC_GPIO_EMC_37_GPIO3_IO23 },
  { IOMUXC_GPIO_EMC_38_GPIO3_IO24 },
  { IOMUXC_GPIO_EMC_39_GPIO3_IO25 },
  { IOMUXC_GPIO_EMC_40_GPIO3_IO26 },
  { IOMUXC_GPIO_EMC_41_GPIO3_IO27 },
  { 0U ,0U ,0U ,0U ,0U },
  { 0U ,0U ,0U ,0U ,0U },
  { 0U ,0U ,0U ,0U ,0U },
  { 0U ,0U ,0U ,0U ,0U },
  { IOMUXC_GPIO_EMC_00_GPIO4_IO00 },
  { IOMUXC_GPIO_EMC_01_GPIO4_IO01 },
  { IOMUXC_GPIO_EMC_02_GPIO4_IO02 },
  { IOMUXC_GPIO_EMC_03_GPIO4_IO03 },
  { IOMUXC_GPIO_EMC_04_GPIO4_IO04 },
  { IOMUXC_GPIO_EMC_05_GPIO4_IO05 },
  { IOMUXC_GPIO_EMC_06_GPIO4_IO06 },
  { IOMUXC_GPIO_EMC_07_GPIO4_IO07 },
  { IOMUXC_GPIO_EMC_08_GPIO4_IO08 },
  { IOMUXC_GPIO_EMC_09_GPIO4_IO09 },
  { IOMUXC_GPIO_EMC_10_GPIO4_IO10 },
  { IOMUXC_GPIO_EMC_11_GPIO4_IO11 },
  { IOMUXC_GPIO_EMC_12_GPIO4_IO12 },
  { IOMUXC_GPIO_EMC_13_GPIO4_IO13 },
  { IOMUXC_GPIO_EMC_14_GPIO4_IO14 },
  { IOMUXC_GPIO_EMC_15_GPIO4_IO15 },
  { IOMUXC_GPIO_EMC_16_GPIO4_IO16 },
  { IOMUXC_GPIO_EMC_17_GPIO4_IO17 },
  { IOMUXC_GPIO_EMC_18_GPIO4_IO18 },
  { IOMUXC_GPIO_EMC_19_GPIO4_IO19 },
  { IOMUXC_GPIO_EMC_20_GPIO4_IO20 },
  { IOMUXC_GPIO_EMC_21_GPIO4_IO21 },
  { IOMUXC_GPIO_EMC_22_GPIO4_IO22 },
  { IOMUXC_GPIO_EMC_23_GPIO4_IO23 },
  { IOMUXC_GPIO_EMC_24_GPIO4_IO24 },
  { IOMUXC_GPIO_EMC_25_GPIO4_IO25 },
  { IOMUXC_GPIO_EMC_26_GPIO4_IO26 },
  { IOMUXC_GPIO_EMC_27_GPIO4_IO27 },
  { IOMUXC_GPIO_EMC_28_GPIO4_IO28 },
  { IOMUXC_GPIO_EMC_29_GPIO4_IO29 },
  { IOMUXC_GPIO_EMC_30_GPIO4_IO30 },
  { IOMUXC_GPIO_EMC_31_GPIO4_IO31 },
  { IOMUXC_SNVS_WAKEUP_GPIO5_IO00 },
  { IOMUXC_SNVS_PMIC_ON_REQ_GPIO5_IO01 },
  { IOMUXC_SNVS_PMIC_STBY_REQ_GPIO5_IO02 }
};

// Default IOMUXC Pin Configuration
static const uint32_t DefaultPinConfig =
  IOMUXC_SW_PAD_CTL_PAD_SRE(0U)   |     // Slew Rate: Slow
  IOMUXC_SW_PAD_CTL_PAD_DSE(6U)   |     // Drive Strength
  IOMUXC_SW_PAD_CTL_PAD_SPEED(2U) |     // Speed: Medium
  IOMUXC_SW_PAD_CTL_PAD_ODE(0U)   |     // Open Drain: Disabled
  IOMUXC_SW_PAD_CTL_PAD_PKE(1U)   |     // Pull/Keep Enable
  IOMUXC_SW_PAD_CTL_PAD_PUE(0U)   |     // Pull/Keep Select
  IOMUXC_SW_PAD_CTL_PAD_PUS(0U)   |     // Pull Up/Down Config
  IOMUXC_SW_PAD_CTL_PAD_HYS(0U);        // Hysteresis: Disabled

// GPIOx IRQ Numbers
static IRQn_Type const GPIOIRQn[2*GPIO_MAX_PORTS] = {
  GPIO1_Combined_0_15_IRQn, GPIO1_Combined_16_31_IRQn,
  GPIO2_Combined_0_15_IRQn, GPIO2_Combined_16_31_IRQn,
  GPIO3_Combined_0_15_IRQn, GPIO3_Combined_16_31_IRQn,
  GPIO4_Combined_0_15_IRQn, GPIO4_Combined_16_31_IRQn,
  GPIO5_Combined_0_15_IRQn, GPIO5_Combined_16_31_IRQn
};

// GPIOx Base Pointers
static GPIO_Type * const GPIOBase[GPIO_MAX_PORTS] = {
  GPIO1, GPIO2, GPIO3, GPIO4, GPIO5
};

// Clock IP Names
static clock_ip_name_t const ClockIP[GPIO_MAX_PORTS] = {
  kCLOCK_Gpio1, kCLOCK_Gpio2, kCLOCK_Gpio3, kCLOCK_Gpio4, kCLOCK_Gpio5
};


// Signal Event callback functions
static ARM_GPIO_SignalEvent_t SignalEvent[GPIO_MAX_PORTS][32];


// Common GPIOx IRQ Handler
static void GPIOx_IRQHandler (uint32_t num) {
  GPIO_Type *gpio = GPIOBase[num];
  uint32_t   ifsr;
  uint32_t   irqc;
  uint32_t   event;
  uint32_t   i;

  ifsr = GPIO_PortGetInterruptFlags(gpio);
  for (i = 0U; i < 32U; i++) {
    if (ifsr & (1U << i)) {
      if (SignalEvent[num][i] != NULL) {
        irqc = GPIO_GetPinConfig(gpio, i);
        switch (irqc) {
          case kGPIO_IntRisingEdge:
            event = ARM_GPIO_EVENT_RISING_EDGE;
            break;
          case kGPIO_IntFallingEdge:
            event = ARM_GPIO_EVENT_FALLING_EDGE;
            break;
          case kGPIO_IntRisingOrFallingEdge:
            event = ARM_GPIO_EVENT_EITHER_EDGE;
            break;
          default:
            event = 0U;
            break;
        }
        if (event != 0U) {
          SignalEvent[num][i]((num << 5) | i, event);
        }
      }
    }
  }
  GPIO_PortClearInterruptFlags(gpio, ifsr);
}

// GPIO1 IRQ Handler
void GPIO1_Combined_0_15_IRQHandler (void) {
  GPIOx_IRQHandler (0U);
}
void GPIO1_Combined_16_31_IRQHandler (void) {
  GPIOx_IRQHandler (0U);
}

// GPIO2 IRQ Handler
void GPIO2_Combined_0_15_IRQHandler (void) {
  GPIOx_IRQHandler (1U);
}
void GPIO2_Combined_16_31_IRQHandler (void) {
  GPIOx_IRQHandler (1U);
}

// GPIO3 IRQ Handler
void GPIO3_Combined_0_15_IRQHandler (void) {
  GPIOx_IRQHandler (2U);
}
void GPIO3_Combined_16_31_IRQHandler (void) {
  GPIOx_IRQHandler (2U);
}

// GPIO4 IRQ Handler
void GPIO4_Combined_0_15_IRQHandler (void) {
  GPIOx_IRQHandler (3U);
}
void GPIO4_Combined_16_31_IRQHandler (void) {
  GPIOx_IRQHandler (3U);
}

// GPIO5 IRQ Handler
void GPIO5_Combined_0_15_IRQHandler (void) {
  GPIOx_IRQHandler( 4U);
}
void GPIO5_Combined_16_31_IRQHandler (void) {
  GPIOx_IRQHandler (4U);
}


// Setup GPIO Interface
static int32_t GPIO_Setup (ARM_GPIO_Pin_t pin, ARM_GPIO_SignalEvent_t cb_event) {
  GPIO_Type *gpio;
  uint32_t   pin_port;
  uint32_t   pin_num;
  int32_t    result = ARM_DRIVER_OK;

  pin_port = pin >> 5U;
  pin_num  = pin & 0x1FU;
  if ((PinMapping[pin_port] & (1U << pin_num)) != 0U) {
    gpio = GPIOBase[pin_port];
    CLOCK_EnableClock(ClockIP[pin_port]);
    GPIO_SetPinInterruptConfig(gpio, pin_num, kGPIO_NoIntmode);
    GPIO_PinSetDirection(gpio, pin_num, kGPIO_DigitalInput);
    IOMUXC_SetPinConfig(0U, 0U, 0U, 0U, PinConfig[pin].configRegister, DefaultPinConfig);
    IOMUXC_SetPinMux(PinConfig[pin].muxRegister, PinConfig[pin].muxMode, 0U, 0U, 0U, 0U);
    if (cb_event != NULL) {
      SignalEvent[pin_port][pin_num] = cb_event;
      GPIO_PortDisableInterrupts(gpio, 1U << pin_num);
      NVIC_EnableIRQ(GPIOIRQn[(pin_port << 1U) + (pin_num >> 4U)]);
    }
  } else {
    result = ARM_GPIO_ERROR_PIN;
  }

  return result;
}

// Set GPIO Direction
static int32_t GPIO_SetDirection (ARM_GPIO_Pin_t pin, ARM_GPIO_DIRECTION direction) {
  GPIO_Type *gpio;
  uint32_t   pin_port;
  uint32_t   pin_num;
  int32_t    result = ARM_DRIVER_OK;

  pin_port = pin >> 5U;
  pin_num  = pin & 0x1FU;
  if ((PinMapping[pin_port] & (1U << pin_num)) != 0U) {
    gpio = GPIOBase[pin_port];
    switch (direction) {
      case ARM_GPIO_INPUT:
        GPIO_PinSetDirection(gpio, pin_num, kGPIO_DigitalInput);
        break;
      case ARM_GPIO_OUTPUT:
        GPIO_PinSetDirection(gpio, pin_num, kGPIO_DigitalOutput);
        break;
      default:
        result = ARM_DRIVER_ERROR_PARAMETER;
        break;
    }
  } else {
    result = ARM_GPIO_ERROR_PIN;
  }
 
  return result;
}

// Set GPIO Output Mode
static int32_t GPIO_SetOutputMode (ARM_GPIO_Pin_t pin, ARM_GPIO_OUTPUT_MODE mode) {
  uint32_t pin_port;
  uint32_t pin_num;
  int32_t  result = ARM_DRIVER_OK;

  pin_port = pin >> 5U;
  pin_num  = pin & 0x1FU;
  if ((PinMapping[pin_port] & (1U << pin_num)) != 0U) {
    switch (mode) {
      case ARM_GPIO_PUSH_PULL:
        IOMUXC_EnableOpenDran(PinConfig[pin].configRegister, false);
        break;
      case ARM_GPIO_OPEN_DRAIN:
        IOMUXC_EnableOpenDran(PinConfig[pin].configRegister, true);
        break;
      default:
        result = ARM_DRIVER_ERROR_PARAMETER;
        break;
    }
  } else {
    result = ARM_GPIO_ERROR_PIN;
  }
 
  return result;
}

// Set GPIO Pull Resistor
static int32_t GPIO_SetPullResistor (ARM_GPIO_Pin_t pin, ARM_GPIO_PULL_RESISTOR resistor) {
  uint32_t pin_port;
  uint32_t pin_num;
  int32_t  result = ARM_DRIVER_OK;

  pin_port = pin >> 5U;
  pin_num  = pin & 0x1FU;
  if ((PinMapping[pin_port] & (1U << pin_num)) != 0U) {
    switch (resistor) {
      case ARM_GPIO_PULL_NONE:
        IOMUXC_SetPinPullConfig(PinConfig[pin].configRegister, kIOMUXC_PullDisable);
        break;
      case ARM_GPIO_PULL_UP:
        IOMUXC_SetPinPullConfig(PinConfig[pin].configRegister, kIOMUXC_PullUp_100K);
        break;
      case ARM_GPIO_PULL_DOWN:
        IOMUXC_SetPinPullConfig(PinConfig[pin].configRegister, kIOMUXC_PullDown_100K);
        break;
      default:
        result = ARM_DRIVER_ERROR_PARAMETER;
        break;
    }
  } else {
    result = ARM_GPIO_ERROR_PIN;
  }
 
  return result;
}

// Set GPIO Event Trigger
static int32_t GPIO_SetEventTrigger (ARM_GPIO_Pin_t pin, ARM_GPIO_EVENT_TRIGGER trigger) {
  GPIO_Type *gpio;
  uint32_t   pin_port;
  uint32_t   pin_num;
  int32_t    result = ARM_DRIVER_OK;

  pin_port = pin >> 5U;
  pin_num  = pin & 0x1FU;
  if ((PinMapping[pin_port] & (1U << pin_num)) != 0U) {
    gpio = GPIOBase[pin_port];
    switch (trigger) {
      case ARM_GPIO_TRIGGER_NONE:
        GPIO_PortDisableInterrupts(gpio, 1U << pin_num);
        GPIO_PinSetInterruptConfig(gpio, pin_num, kGPIO_NoIntmode);
        break;
      case ARM_GPIO_TRIGGER_RISING_EDGE:
        GPIO_PinSetInterruptConfig(gpio, pin_num, kGPIO_IntRisingEdge);
        GPIO_PortEnableInterrupts(gpio, 1U << pin_num);
        break;
      case ARM_GPIO_TRIGGER_FALLING_EDGE:
        GPIO_PinSetInterruptConfig(gpio, pin_num, kGPIO_IntFallingEdge);
        GPIO_PortEnableInterrupts(gpio, 1U << pin_num);
        break;
      case ARM_GPIO_TRIGGER_EITHER_EDGE:
        GPIO_PinSetInterruptConfig(gpio, pin_num, kGPIO_IntRisingOrFallingEdge);
        GPIO_PortEnableInterrupts(gpio, 1U << pin_num);
        break;
      default:
        result = ARM_DRIVER_ERROR_PARAMETER;
        break;
    }
  } else {
    result = ARM_GPIO_ERROR_PIN;
  }
 
  return result;
}

// Set GPIO Output Level
static void GPIO_SetOutput (ARM_GPIO_Pin_t pin, uint32_t val) {
  GPIO_Type *gpio;
  uint32_t   pin_port;
  uint32_t   pin_num;

  if (pin < GPIO_MAX_PINS) {
    pin_port = pin >> 5U;
    pin_num  = pin & 0x1FU;
    gpio = GPIOBase[pin_port];
    GPIO_PinWrite(gpio, pin_num, (uint8_t)val);
  }
}

// Get GPIO Input Level
static uint32_t GPIO_GetInput (ARM_GPIO_Pin_t pin) {
  GPIO_Type *gpio;
  uint32_t   pin_port;
  uint32_t   pin_num;
  uint32_t   val = 0U;

  if (pin < GPIO_MAX_PINS) {
    pin_port = pin >> 5U;
    pin_num  = pin & 0x1FU;
    gpio = GPIOBase[pin_port];
    val = GPIO_PinRead(gpio, pin_num);
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
