/*
  gpio.c

   Created on: Dec 12, 2018
       Author: Dan Walkes
   Updated by Dave Sluiter Dec 31, 2020. Minor edits with #defines.

   March 17
   Dave Sluiter: Use this file to define functions that set up or control GPIOs.

   Jan 24, 2023
   Dave Sluiter: Cleaned up gpioInit() to make it less confusing for students regarding
                 drive strength setting.

 *
 * Student edit:
 * @student    Samiksha Patil, Samiksha.Patil@Colorado.edu
 *

 */

// *****************************************************************************
// Students:
// We will be creating additional functions that configure and manipulate GPIOs.
// For any new GPIO function you create, place that function in this file.
// *****************************************************************************

#include <string.h>
#include "gpio.h"

// Student Edit: Define these, 0's are placeholder values.
//
// See the radio board user guide at https://www.silabs.com/documents/login/user-guides/ug279-brd4104a-user-guide.pdf
// and GPIO documentation at https://siliconlabs.github.io/Gecko_SDK_Doc/efm32g/html/group__GPIO.html
// to determine the correct values for these.
// If these links have gone bad, consult the reference manual and/or the datasheet for the MCU.
// Change to correct port and pins:

// Set GPIO drive strengths and modes of operation
void gpioInit()
{
    // Student Edit:

    // Set the port's drive strength. In this MCU implementation, all GPIO cells
    // in a "Port" share the same drive strength setting.
    // GPIO_DriveStrengthSet(LED_port, gpioDriveStrengthStrongAlternateStrong); // Strong, 10mA
    GPIO_DriveStrengthSet(LED_port, gpioDriveStrengthWeakAlternateWeak); // Weak, 1mA

    // Set the 2 GPIOs mode of operation
    GPIO_PinModeSet(LED_port, LED0_pin, gpioModePushPull, false);
    GPIO_PinModeSet(LED_port, LED1_pin, gpioModePushPull, false);

    // Configure PB0 as an input with pull-up resistor
    GPIO_PinModeSet(BUTTON_ENABLE_PORT, BUTTON0_PIN, gpioModeInputPull, 1);

    // Enable interrupt (button press)
    GPIO_ExtIntConfig(BUTTON_ENABLE_PORT, BUTTON0_PIN, BUTTON0_PIN, true, true, true);

    // Enable EVEN IRQ for GPIO
    NVIC_EnableIRQ(GPIO_EVEN_IRQn);

    // Configure PB0 as an input with pull-up resistor
    GPIO_PinModeSet(BUTTON_ENABLE_PORT, BUTTON1_PIN, gpioModeInputPull, 1);

    // Enable interrupt (button press)
    GPIO_ExtIntConfig(BUTTON_ENABLE_PORT, BUTTON1_PIN, BUTTON1_PIN, true, true, true);

    // Set the pin as push-pull output
        GPIO_PinModeSet(BUZZER_PORT, BUZZER_PIN, gpioModePushPull, 0);

    // Enable EVEN IRQ for GPIO
    NVIC_EnableIRQ(GPIO_ODD_IRQn);

} // gpioInit()

void gpioLed0SetOn()
{
    GPIO_PinOutSet(LED_port, LED0_pin);
}

void gpioLed0SetOff()
{
    GPIO_PinOutClear(LED_port, LED0_pin);
}

void gpioLed1SetOn()
{
    GPIO_PinOutSet(LED_port, LED1_pin);
}

void gpioLed1SetOff()
{
    GPIO_PinOutClear(LED_port, LED1_pin);
}

void gpioLed0Toggle()
{
    GPIO_PinOutToggle(LED_port, LED0_pin);
}

void gpioLed1Toggle()
{
    GPIO_PinOutToggle(LED_port, LED1_pin);
}

void gpioI2cSclOn()
{
    GPIO_PinOutSet(I2C_SCL_PORT, I2C_SCL_PIN);
}

void gpioI2cSclOff()
{
    GPIO_PinOutClear(I2C_SCL_PORT, I2C_SCL_PIN);
}

void gpioI2cSdaOn()
{
    GPIO_PinOutSet(I2C_SDA_PORT, I2C_SDA_PIN);
}

void gpioI2cSdaOff()
{
    GPIO_PinOutClear(I2C_SDA_PORT, I2C_SDA_PIN);
}

void gpioSensorOn()
{
    GPIO_PinOutSet(SENSOR_ENABLE_PORT, SENSOR_ENABLE_PIN);
}

void gpioSensorOff()
{
    GPIO_PinOutClear(SENSOR_ENABLE_PORT, SENSOR_ENABLE_PIN);
}

void gpioSetDisplayExtcomin(bool previous_state)
{
    if (previous_state)
    {
        GPIO_PinOutClear(SENSOR_ENABLE_PORT, EXTCOMIN_PIN); // Set LOW if previous state was HIGH
    }
    else
    {
        GPIO_PinOutSet(SENSOR_ENABLE_PORT, EXTCOMIN_PIN); // Set HIGH if previous state was LOW
    }
}

void buzzer_on(void)
{
    GPIO_PinOutSet(BUZZER_PORT, BUZZER_PIN); // Output HIGH
}

void buzzer_off(void)
{
    GPIO_PinOutClear(BUZZER_PORT, BUZZER_PIN); // Output LOW
}
