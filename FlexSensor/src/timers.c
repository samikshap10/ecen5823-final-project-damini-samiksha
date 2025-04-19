
/***********************************************************************
 * @file      timers.c
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
#include <src/timers.h>
#include <src/gpio.h>
#include "em_letimer.h"
#include "app.h"
#include "src/timers.h"

#define INCLUDE_LOG_DEBUG   1
#include "src/log.h"


#define ACTUAL_CLOCK_FREQ (OSC_FREQ/PRESCALER_VAL) // The actual clock frequency to load LETIMER0
#define LETIMER_COMP0_VAL   ((LETIMER_PERIOD_MS * ACTUAL_CLOCK_FREQ)/1000) // The value for COMP0 (period of the timer)
#define LETIMER_COMP1_VAL ((LETIMER_ON_TIME_MS * ACTUAL_CLOCK_FREQ)/1000) // The value for COMP1 (on-time of the timer)

/**
 * @brief Initializes LETIMER0 with the given configuration.
 *
 * This function configures the LETIMER0 peripheral by setting up the
 * appropriate settings in the LETIMER_Init_TypeDef structure.
 * The COMP0 and COMP1 values are also set to determine the timer period and on-time.
 *
 * @param None
 * @return None
 *
 */
void LETIMER0Init(void)
{
  LETIMER_Init_TypeDef LETIMER0_Init_Struct;

  // Initialize the LETIMER0_Init_Struct to configure the LETIMER0 peripheral
  LETIMER0_Init_Struct.enable   = false; // Disable LETIMER initially; we'll enable it later
  LETIMER0_Init_Struct.debugRun = false; // Do not let the timer run while the debugger is attached
  LETIMER0_Init_Struct.comp0Top = true;  // Set COMP0 to be the top value; the timer will count down to COMP0 and reset to 0
  LETIMER0_Init_Struct.bufTop   = false; // Disable the buffer for COMP0 (do not use buffered register for the top value)
  LETIMER0_Init_Struct.out0Pol  = false; // Set output 0 to be inactive (low) when the timer reaches COMP0
  LETIMER0_Init_Struct.out1Pol  = false; // Set output 1 to be inactive (low) when the timer reaches COMP1
  LETIMER0_Init_Struct.ufoa0    = letimerUFOANone; // No output action on underflow for output 0
  LETIMER0_Init_Struct.ufoa1    = letimerUFOANone; // No output action on underflow for output 1
  LETIMER0_Init_Struct.repMode  = letimerRepeatFree; // Timer will keep running freely (no repetition limit)
  LETIMER0_Init_Struct.topValue = false; // Don't use an external value for the top value, rely on COMP0

  // Initialize LETIMER0 peripheral with Init Struct values
  LETIMER_Init(LETIMER0, &LETIMER0_Init_Struct);

  // Set COMP0 value to required value i.e LETIMER_PERIOD_MS
  LETIMER_CompareSet(LETIMER0, 0, LETIMER_COMP0_VAL);
}

/**
 * @brief Enables interrupts for LETIMER0 and configures NVIC.
 *
 * This function enables interrupts for COMP0, COMP1, and Underflow
 * events of LETIMER0. It clears any pending interrupts and then
 * enables interrupts in the NVIC to handle LETIMER0 interrupts.
 *
 * @param None
 * @return None
 *
 */
void LETIMER0EnableIrq(void)
{
  // Enable interrupts for UF
  LETIMER_IntEnable(LETIMER0,LETIMER_IEN_UF);

  // Clear any pending IRQ
  NVIC_ClearPendingIRQ(LETIMER0_IRQn);

  // Enable Interrupts in the NVIC for LETIMER0
  NVIC_EnableIRQ(LETIMER0_IRQn);

  // Enable the LETIMER0 peripheral
  LETIMER_Enable(LETIMER0, SET);
}

/**
 * @brief Waits for a specified number of microseconds using the LETIMER with proper wraparound handling.
 *
 * This function calculates the number of timer ticks corresponding to the requested delay in microseconds
 * and waits until the LETIMER counter reaches the target value. It handles both normal and wraparound cases
 * of the timer counter. The function performs input range checking and adjusts the delay if necessary.
 *
 * @param[in] us_wait The number of microseconds to wait. If the value is out of the valid range, it is clamped
 *                    to the maximum valid delay (UINT16_MAX).
 *
 * @note If the requested delay exceeds the maximum valid range for the timer counter, the function will adjust
 *       the delay to the maximum allowable value.
 *
 * @note The delay is calculated based on a constant factor `COUNT_PER_US` that maps microseconds to timer ticks.
 *       The wraparound behavior of the timer is also handled by comparing the timer's counter to the
 *       `LETIMER_COMP0_VAL`.
 */
void timerWaitUs_poll(uint32_t us_wait){

  uint32_t counterVal = LETIMER_CounterGet(LETIMER0);
  uint32_t counterWait = (us_wait * COUNT_PER_US) / 1000;
  uint16_t delayTick;

  //range checking the input
  if(counterWait > UINT16_MAX || us_wait < ACTUAL_CLOCK_FREQ){
    us_wait = UINT16_MAX;
    LOG_WARN("Provided delay is out of range!\r\n");
  }

  counterWait = (us_wait * COUNT_PER_US) / 1000;

  // Normal case: calculate delay ticks
  delayTick = counterVal - counterWait;

  // Wraparound case: handle if the delay ticks exceed the counter range
  if(delayTick > LETIMER_COMP0_VAL)
    delayTick = LETIMER_COMP0_VAL - (counterWait - counterVal);

  // Wait until the LETIMER counter reaches the target value
  while(LETIMER_CounterGet(LETIMER0) != delayTick){

  }
}

/**
 * @brief Generates a delay in microseconds using LETIMER with an interrupt.
 *
 * This function sets up a delay in microseconds using the LETIMER peripheral.
 * It calculates the required delay ticks and configures the COMP1 register to
 * trigger an interrupt after the specified time. It also handles wrap-around cases.
 *
 * @param us_wait The delay time in microseconds.
 */
void timerWaitUs_irq(uint32_t us_wait){

  uint32_t counterVal = LETIMER_CounterGet(LETIMER0);
  uint32_t counterWait = (us_wait * COUNT_PER_US) / 1000;
  uint16_t delayTick;

  //range checking the input
  if(counterWait > UINT16_MAX || us_wait < ACTUAL_CLOCK_FREQ){
    us_wait = UINT16_MAX;
    LOG_WARN("Provided delay is out of range!\r\n");
  }

  counterWait = (us_wait * COUNT_PER_US) / 1000;

  //Normal case
  delayTick = counterVal - counterWait;
  //Wrap around case
  if(delayTick > LETIMER_COMP0_VAL)
    delayTick = LETIMER_COMP0_VAL - (counterWait - counterVal);

  //Load the COMP1 value
  LETIMER_CompareSet(LETIMER0, 1, delayTick);
  //Enable the COMP1 interrupt
  LETIMER_IntEnable(LETIMER0, LETIMER_IEN_COMP1);

}

/**
 * @brief Unit test for the timerWaitUs_poll function.
 *
 * This test function exercises the `timerWaitUs_poll` function under various conditions:
 * - Minimum delay check: Verifies the behavior when the delay is at the lower bound.
 * - Normal case: Verifies the behavior for typical delays.
 * - Range check: Verifies the behavior when the delay exceeds the maximum allowable range.
 *
 * The test toggles the GPIO LED states to provide a visual indication of the progress through the test.
 */
void timerWaitUs_poll_UnitTest(void){

  timerWaitUs_poll(999);//Minimum delay check
  gpioLed0SetOn();
  timerWaitUs_poll(500000); // Normal case
  gpioLed0SetOff();
  timerWaitUs_poll(500000); // Normal case
  gpioLed0SetOn();
  timerWaitUs_poll(500000); // Normal case
  gpioLed0SetOff();
  timerWaitUs_poll(500000); // Normal case
  timerWaitUs_poll(70000000);//Range check

}

/**
 * @brief Unit test for the timerWaitUs_irq function.
 *
 * This test function exercises the `timerWaitUs_irq` function under various conditions:
 * - Minimum delay check: Verifies the behavior when the delay is at the lower bound.
 * - Normal case: Verifies the behavior for typical delays.
 * - Range check: Verifies the behavior when the delay exceeds the maximum allowable range.
 *
 * The test toggles the GPIO LED states to provide a visual indication of the progress through the test.
 */
void timerWaitUs_irq_UnitTest(Events_t evt){

  static uint8_t flag =0;
  if((evt == EVENT_LETIMER_COMP1)&&(flag == 0))
    {
      flag++;
      gpioLed0Toggle();
      timerWaitUs_irq(500000);//Normal case
    }
  else if((evt == EVENT_LETIMER_COMP1)&&(flag == 1))
    {
      flag++;
      gpioLed0Toggle();
      timerWaitUs_irq(999); // Minimum delay check
    }
  else if((evt == EVENT_LETIMER_COMP1)&&(flag == 2))
    {
      flag = 0;
      gpioLed0Toggle();
      timerWaitUs_irq(70000000); // Range check
    }

}
