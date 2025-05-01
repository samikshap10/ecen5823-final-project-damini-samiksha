/*
  File: oscillators.c

  Author: Samiksha Patil
  Description:
   This file (oscillators.c) contains the implementation of the oscillator initialization function
   for configuring the clocks and oscillators on the device, particularly focusing
   on enabling the Ultra Low-Frequency Oscillator (ULFRCO) for energy-efficient
   operation in Low Energy Modes (EM3) and setting up the Low Energy Timer (LETIMER0).
   The clock settings are optimized for low power consumption.
  References:
  - Silicon Labs, "EFM32 Low Energy Microcontrollers," Application Note AN0032, 2015.
  - Silicon Labs, "Energy Modes and Clock System for EFM32," EFM32 Reference Manual, 2018.
  - Lecture Slides

*/

#include <src/oscillators.h>
#include "em_cmu.h"

/**
 * @brief Initializes oscillators and clock sources for low-power operation.
 * 
 * This function enables the Ultra Low-Frequency Oscillator (ULFRCO), selects it as the clock 
 * source for the Low-Frequency A (LFA) clock tree, and configures the Low Energy Timer 
 * (LETIMER0).
 * Keeping only one clock for all modes so that it will turn on from sleep. 
 * 
 * @param None
 * 
 * @return None
 */
void oscillatorsInit(void)
{
    // Enable the Ultra Low-Frequency Oscillator (ULFRCO) for EM3 operation
    CMU_OscillatorEnable(cmuOsc_ULFRCO, true, true);

    // Select ULFRCO as the clock source for the Low-Frequency A (LFA) clock tree
    CMU_ClockSelectSet(cmuClock_LFA, cmuSelect_ULFRCO);
    
     // Set a clock divider of 4 for LETIMER0 to reduce power consumption
    CMU_ClockDivSet(cmuClock_LETIMER0, cmuClkDiv_1);

    // Enable the clock for the Low Energy Timer (LETIMER0)
    CMU_ClockEnable(cmuClock_LETIMER0, true);

}
