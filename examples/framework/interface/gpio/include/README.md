# GPIO Driver API Proposal

Following is an API proposal for GPIO (General-purpose Input/Output) Driver. 
The API features I/O operations on pin level (does not support simultaneous operations on multiple pins belonging to the same port). 
The API provides basic pin configuration (direction, output mode, pull-resistor, event trigger) excluding advanced settings (drive strength or speed, input filter, ...), 
setting outputs and reading inputs.

Current proposal is the [API not aligned with CMSIS-Driver specification](#api-not-aligned-with-cmsis-driver-specification).

The API is defined in the [Driver_GPIO.h](./Driver_GPIO.h) header file.


## API aligned with CMSIS-Driver specification

This proposal is aligned with [CMSIS-Driver](https://arm-software.github.io/CMSIS_5/Driver/html/group__mci__interface__gr.html) specification with some exceptions.

The API provides common driver functions and expects function calls as described in CMSIS-Driver [Theory of Operation](https://arm-software.github.io/CMSIS_5/Driver/html/theoryOperation.html).

>Note: The term GPIO is used consistently in the API despite that it defines operations on pin level.

### Pin Identification

Each function (except **GetVersion**) operates on a pin level and uses a pin identification as the first parameter. 
Pin identification is a virtual number (uint32_t) which is mapped to an actual pin.

### Common Driver Functions

 - **GetVersion** [Aligned with CMSIS-Driver specification]
 - **GetCapabilities**: Currently not specified but could be introduced with specifying features (Open-drain, Pull-up/down, Trigger events).
 - **Initialize** [Aligned with CMSIS-Driver specification]: Must be called before powering the peripheral using **PowerControl**. It only initializes driver software 
   if needed (allocates resources but does not configure the peripheral itself) and registers the optional **SignalEvent** callback function.
 - **Uninitialize** [Aligned with CMSIS-Driver specification]: Complementary to **Initialize**. Releases resources used by the driver.
 - **PowerControl** [Aligned with CMSIS-Driver specification]: Controls the power profile of the peripheral and needs to be called after **Initialize**. 
   Power options: 
    - ARM_POWER_FULL: Peripheral is turned on and fully operational. The driver enables clock, configures the pin multiplexer and interrupts. 
      The pin is setup with the default configuration (Direction: Input, Output Mode: Push-pull, Pull-resistor: None, Event Trigger: None).
    - ARM_POWER_LOW: not supported
    - ARM_POWER_OFF: Peripheral is turned off and not operational. The pin is disabled (clock, multiplexer, interrupts).
 - **Control**: Currently not specified but could be introduced to control the pin and replace the following functions: **SetDirection**, **SetOutputMode**, **SetPullResistor** 
   and **SetEventTrigger**.
   >Note: Using **Control** function provides also a backward compatible way to extend the API (example: adding control for drive strength, ...)
 - **SignalEvent** [Aligned with CMSIS-Driver specification]: Optional callback function that is registered with the **Initialize** function. 
   Called from interrupt service routine when configured event is triggered by the peripheral.

>Note: Specifying a pin parameter in Common Driver Functions is a deviation from CMSIS-Driver specification.

### GPIO Driver Specific Functions

#### Configure Pin

The following functions configure the pin direction, output mode, pull resistor and trigger event:
 - **SetDirection**: Input (default), Output
 - **SetOutputMode**: Push-pull (default), Open-drain
 - **SetPullResistor**: None (default), Pull-up, Pull-down
 - **SetEventTrigger**: None (default), Rising-edge, Falling-edge, Either edge (rising and falling)

Each function controls just one peripheral property which is typically aligned with peripheral registers.

Alternative could be a single function which configures all properties. However the driver implementation needs to be correctly implemented 
in order to avoid potential unwanted glitches on pins or spurious interrupts. Example: Switching from output with high-level to input with pull-up resistor 
might introduce a low-level glitch on the pin and a spurious event if the driver implementation is not configuring the peripheral registers in the correct order.

#### Read/Write Pin

The following functions set the pin output or read the pin input:
 - **SetOutput**
 - **GetInput**

 >Note: **SetOutput** can be used also in input mode to set the output register to desired value before the direction is changed to output.

 
## API not aligned with CMSIS-Driver specification

Proposed API for GPIO Driver is fairly simple. Using CMSIS-Driver Common Functions might introduce unnecessary overhead and complexity. 
Alternative is a simpler API which is however not aligned with CMSIS-Driver specification.

CMSIS-Driver Common Functions (**Initialize**, **Uninitialize**, **PowerControl**) are replaced by a single function **Setup** 
which behaves as combined **Initialize** and **PowerControl**(ARM_POWER_FULL).

Other Common Functions (**GetVersion**, **GetCapabilities**) are dropped for further simplification.

The API consists of the following functions:
```
int32_t  (*Setup)           (ARM_GPIO_Pin_t pin, ARM_GPIO_SignalEvent_t cb_event)
int32_t  (*SetDirection)    (ARM_GPIO_Pin_t pin, ARM_GPIO_DIRECTION direction)
int32_t  (*SetOutputMode)   (ARM_GPIO_Pin_t pin, ARM_GPIO_OUTPUT_MODE mode)
int32_t  (*SetPullResistor) (ARM_GPIO_Pin_t pin, ARM_GPIO_PULL_RESISTOR resistor)
int32_t  (*SetEventTrigger) (ARM_GPIO_Pin_t pin, ARM_GPIO_EVENT_TRIGGER trigger) 
void     (*SetOutput)       (ARM_GPIO_Pin_t pin, uint32_t level)
uint32_t (*GetInput)        (ARM_GPIO_Pin_t pin)
void     (*SignalEvent)     (ARM_GPIO_Pin_t pin, uint32_t event);  // Callback
```

>Note: Functions are still exposed in an access structure as in CMSIS-Driver (could be changed).
