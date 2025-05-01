/*
   gpio.h

    Created on: Dec 12, 2018
        Author: Dan Walkes

    Updated by Dave Sluiter Sept 7, 2020. moved #defines from .c to .h file.
    Updated by Dave Sluiter Dec 31, 2020. Minor edits with #defines.

    Editor: Feb 26, 2022, Dave Sluiter
    Change: Added comment about use of .h files.

 *
 * Student edit: Add your name and email address here:
 * @student    Awesome Student, Awesome.Student@Colorado.edu
 *

 */

// Students: Remember, a header file (a .h file) generally defines an interface
//           for functions defined within an implementation file (a .c file).
//           The .h file defines what a caller (a user) of a .c file requires.
//           At a minimum, the .h file should define the publicly callable
//           functions, i.e. define the function prototypes. #define and type
//           definitions can be added if the caller requires theses.

#ifndef SRC_GPIO_H_
#define SRC_GPIO_H_

#include <stdbool.h>
#include "sl_bt_api.h"
#include "em_gpio.h"
#include <src/irq.h>

#define LED_port (gpioPortF)
#define LED0_pin (4)
#define LED1_pin (5)

// Define GPIO ports and pins for I2C and sensor power
#define I2C_SCL_PORT gpioPortC
#define I2C_SCL_PIN 10
#define I2C_SDA_PORT gpioPortC
#define I2C_SDA_PIN 11
#define SENSOR_ENABLE_PORT gpioPortD
#define SENSOR_ENABLE_PIN 15
#define EXTCOMIN_PIN 13
#define BUTTON_ENABLE_PORT gpioPortF
#define BUTTON0_PIN 6
#define BUTTON1_PIN 7
#define BUZZER_PORT gpioPortC
#define BUZZER_PIN  11

#define PB0_EXT_SIGNAL (1 << 4) // External Signal ID
#define PB1_EXT_SIGNAL (1 << 5) // External Signal ID

// Function prototypes
// Led functions to turn ON, OFF, TOGGLE
void gpioInit();
void gpioLed0SetOn();
void gpioLed0SetOff();
void gpioLed1SetOn();
void gpioLed1SetOff();
void gpioLed0Toggle();
void gpioLed1Toggle();

// I2C ON OFF functions
void gpioI2cSclOn();
void gpioI2cSclOff();
void gpioI2cSdaOn();
void gpioI2cSdaOff();
void buzzer_on(void);
void buzzer_off(void);

// Sensor ON OFF functions
void gpioSensorOn();
void gpioSensorOff();

// Function to toggle the EXTCOMIN pin of LCD
void gpioSetDisplayExtcomin(bool previous_state);

#endif /* SRC_GPIO_H_ */
