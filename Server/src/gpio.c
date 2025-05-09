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
 * Student edit: Add your name and email address here:
 * @student    Damini Gowda, damini.gowda@Colorado.edu
 *
 
 */


// *****************************************************************************
// Students:
// We will be creating additional functions that configure and manipulate GPIOs.
// For any new GPIO function you create, place that function in this file.
// *****************************************************************************

#include <stdbool.h>
#include "em_gpio.h"
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
	GPIO_DriveStrengthSet(LED_port, gpioDriveStrengthStrongAlternateStrong); // Strong, 10mA
	//GPIO_DriveStrengthSet(LED_port, gpioDriveStrengthWeakAlternateWeak); // Weak, 1mA
	
	// Set the 2 GPIOs mode of operation
	GPIO_PinModeSet(LED_port, LED0_pin, gpioModePushPull, false);
	GPIO_PinModeSet(LED_port, LED1_pin, gpioModePushPull, false);

	//Init Si7021 GPIO
	  GPIO_PinModeSet(SI7021_port, SI7021_pin, gpioModePushPull, true);

	  //Init LCD ExtComin Pin
	  GPIO_PinModeSet(LCD_port, LCD_EXTCOMIN_pin, gpioModePushPull, false);

	  GPIO_PinModeSet(PB0_PORT, PB0_PIN, gpioModeInputPull, 1);
	  GPIO_ExtIntConfig(PB0_PORT, PB0_PIN, PB0_PIN, true, true, true);

	  // Configure the pin as input with pull-up (or pull-down as needed)
	  GPIO_PinModeSet(SENSOR_ENABLE_PORT, ACC_INT_PIN, gpioModeInputPull, 0);
	  // Enable GPIO interrupt for both rising and falling edges
	  GPIO_ExtIntConfig(SENSOR_ENABLE_PORT, ACC_INT_PIN, ACC_INT_PIN, true, false, true);

} // gpioInit()


void gpioLed0SetOn()
{
	GPIO_PinOutSet(LED_port, LED0_pin);
}


void gpioLed0SetOff()
{
	GPIO_PinOutClear(LED_port, LED0_pin);
}

void gpioLed0Toggle()
{
  GPIO_PinOutToggle(LED_port, LED0_pin);
}


void gpioLed1SetOn()
{
	GPIO_PinOutSet(LED_port, LED1_pin);
}


void gpioLed1SetOff()
{
	GPIO_PinOutClear(LED_port, LED1_pin);
}

void gpioSi7021Enable()
{
  GPIO_PinOutSet(SI7021_port,SI7021_pin);
}

void gpioSi7021Disable()
{
  GPIO_PinOutClear(SI7021_port,SI7021_pin);
}

void gpioI2CSDADisable()
{
  //GPIO_PinModeSet(SI7021_SDA_port, SI7021_SDA_pin, gpioModeWiredAndPullUp, 1);
  GPIO_PinOutClear(SI7021_SDA_port, SI7021_SDA_pin);
}

void gpioI2CSCLToggle()
{
  GPIO_PinOutToggle(SI7021_SCL_port, SI7021_SCL_pin);
}

void gpioI2CSCLDisable()
{
  //GPIO_PinModeSet(SI7021_SCL_port, SI7021_SCL_pin, gpioModeWiredAndPullUp, 1);
  GPIO_PinOutClear(SI7021_SCL_port, SI7021_SCL_pin);
}

void gpioI2CSCLEnable()
{
  GPIO_PinModeSet(SI7021_SCL_port, SI7021_SCL_pin, gpioModeWiredAndPullUp, 1);
  GPIO_PinOutSet(SI7021_SCL_port, SI7021_SCL_pin);
}

void gpioI2CSDAEnable()
{
  GPIO_PinModeSet(SI7021_SDA_port, SI7021_SDA_pin, gpioModeWiredAndPullUp, 1);
  GPIO_PinOutSet(SI7021_SDA_port, SI7021_SDA_pin);
}

void gpioSetDisplayExtcomin(bool curr_state)
{
  if(curr_state == true){
      GPIO_PinOutSet(LCD_port, LCD_EXTCOMIN_pin);
  }else{
      GPIO_PinOutClear(LCD_port, LCD_EXTCOMIN_pin);
  }

}

