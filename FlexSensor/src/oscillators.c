
/***********************************************************************
 * @file      oscillators.c
 * @version   0.1
 * @brief     Function header/interface file.
 *
 * @author    Damini Gowda, damini.gowda@colorado.edu
 * @date      Jan 26, 2025
 *
 *
 * @institution University of Colorado Boulder (UCB)
 * @course      ECEN 5823-001: IoT Embedded Firmware
 * @instructor  Chris Choi
 *
 * @assignment Assignment 2 - Managing Energy Modes
 * @due
 *
 * @resources EFR32xG13 Wireless GeckoReference Manual, Lecture PDFs and reading references
 *
 */

#include <src/oscillators.h>
#include "em_cmu.h"
#include "app.h"

/**
 * @brief Initializes the Clock Management Unit (CMU) for the low-energy mode.
 *
 * This function initializes the CMU to select the appropriate clock source
 * for the specified energy mode. It configures the oscillator (ULFRCO for
 * EM3 or LFXO for other modes), sets the clock branch, and configures the
 * prescaler for LETIMER0. It also enables the clock for LETIMER0.
 *
 * @note The clock configuration for LETIMER0 and LFA is specific to the
 *       selected energy mode.
 *
 * @param None
 * @return None
 */
void CMU_init(void){

#if (LOWEST_ENERGY_MODE == EM3)

  // Enable ULFRCO Oscillator, wait for clock to stabilize
  CMU_OscillatorEnable(cmuOsc_ULFRCO, SET, SET);

  // Select LFA Clock branch and ULFRCO Clock reference
  CMU_ClockSelectSet(cmuClock_LFA, cmuSelect_ULFRCO);

  // Set Clock divider to 1
  CMU_ClockDivSet(cmuClock_LETIMER0, PRESCALER_VAL);

  // Enable the LETIMER0
  CMU_ClockEnable(cmuClock_LETIMER0, SET);

#else

  // Enable LFXO Oscillator, wait for clock to stabilize
  CMU_OscillatorEnable(cmuOsc_LFXO, SET, SET);

  // Select LFA Clock branch and LFXO Clock reference
  CMU_ClockSelectSet(cmuClock_LFA, cmuSelect_LFXO);

  // Set Clock divider to 4
  CMU_ClockDivSet(cmuClock_LETIMER0, PRESCALER_VAL);

  // Enable the LETIMER0
  CMU_ClockEnable(cmuClock_LETIMER0, SET);

  CMU_ClockEnable(cmuClock_GPIO, true);

  // Enable ADC0 clock
  CMU_ClockEnable(cmuClock_ADC0, true);

#endif

}
