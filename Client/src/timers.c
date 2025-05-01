/*
  File: timers.c

  Author: Samiksha Patil
  Description:
   This file (timers.c) contains the implementation of timer-related functions using LETIMER0.
   The functions initialize LETIMER0 with the appropriate settings for low-power operation,
   provide a delay function (`timerWaitUs`) for microsecond-level waiting, and include a unit test
   function for validating the `timerWaitUs` functionality with various test cases.
*/

#include "timers.h"
#include "em_letimer.h"
#include "em_cmu.h"
#include "em_core.h"
#include "app.h"
#define INCLUDE_LOG_DEBUG 1
#include "src/log.h"
#include "src/gpio.h"
/**
 * @brief Initializes LETIMER0 for low-power operation.
 *
 * This function configures the LETIMER0 with the appropriate settings such as enabling the timer,
 * setting the compare values, clearing interrupt flags, and enabling interrupts for underflow (UF).
 * It prepares the timer for low-power operation in the system.
 *
 * @param None
 *
 * @return None
 */
void letimerInit(void)
{
    // The InitData is taken from the Guidance part from the lecture slides :
    uint32_t temp;
    // this data structure is passed to LETIMER_Init (), used to set LETIMER0_CTRL reg bits and other values
    const LETIMER_Init_TypeDef letimerInitData = {
        false,             // enable; don't enable when init completes, we'll enable last
        true,              // debugRun; useful to have the timer running when single-stepping in the debugger
        true,              // comp0Top; load COMP0 into CNT on underflow
        false,             // bufTop; don't load COMP1 into COMP0 when REP0==0
        0,                 // out0Pol; 0 default output pin value
        0,                 // out1Pol; 0 default output pin value
        letimerUFOANone,   // ufoa0; no underflow output action
        letimerUFOANone,   // ufoa1; no underflow output action
        letimerRepeatFree, // repMode; free running mode i.e. load & go forever
        0                  // COMP0(top) Value, I calculate this below
    };

    // init the timer
    LETIMER_Init(LETIMER0, &letimerInitData);

    // Setting the comp values
    LETIMER_CompareSet(LETIMER0, 0, PERIOD_TICKS);
    // LETIMER_CompareSet(LETIMER0, 1, 1000);

    // Clear all IRQ flags in the LETIMER0 IF status register
    LETIMER_IntClear(LETIMER0, 0xFFFFFFFF); // punch them all down
    // Set UF and COMP1 in LETIMER0_IEN, so that the timer will generate IRQs to the NVIC.
    temp = LETIMER_IEN_COMP0;
    LETIMER_IntEnable(LETIMER0, temp); // Make sure you have defined the ISR routine LETIMER0_IRQHandler()
    // Enable the timer to starting counting down, set LETIMER0_CMD[START] bit, see LETIMER0_STATUS[RUNNING] bit
    LETIMER_Enable(LETIMER0, true);
    // Test code:
    // // read it a few times to make sure it's running within the range of values we expect
    temp = LETIMER_CounterGet(LETIMER0);
    temp = LETIMER_CounterGet(LETIMER0);
    temp = LETIMER_CounterGet(LETIMER0);
}

/**
 * @brief Waits for the specified number of microseconds using LETIMER0.
 *
 * This function uses LETIMER0 to implement a delay in microseconds by converting the
 * requested wait time into LETIMER0 ticks. It waits until the LETIMER0 counter reaches the target
 * tick value.
 *
 * @param us_wait The number of microseconds to wait
 *
 * @return None
 */
void timerWaitUs_polled(uint32_t us_wait)
{
    /* Convert microseconds to timer ticks. Instead of using the full formula (us_wait * clock_freq / 1000000),
    we divide by 1000 to simplify the calculation since the clock frequency is fixed at 1000 Hz. */
    uint32_t ticks_to_wait = (us_wait / 1000);
    uint32_t start_tick = LETIMER_CounterGet(LETIMER0);
    uint16_t delay_tick;

    // range checking the input
    if (ticks_to_wait > MAX_COUNTER_VALUE || us_wait < MIN_WAIT_US)
    {
        us_wait = MAX_COUNTER_VALUE;

#ifdef NO_MEASUREMENTS
        LOG_ERROR("Requested wait time is out of range!\r\n");
#endif
    }

    ticks_to_wait = (us_wait / 1000);

    delay_tick = start_tick - ticks_to_wait;

    if (delay_tick > PERIOD_TICKS)
    {
        delay_tick = PERIOD_TICKS - (ticks_to_wait - start_tick);
    }

    // Wait until the counter reaches or passes the delay tick (taking underflow into account)
    while (LETIMER_CounterGet(LETIMER0) != delay_tick)
    {
    }
}

void timerWaitUs_irq(uint32_t us_wait)
{

    uint32_t ticks_to_wait = (us_wait / 1000);

    // range checking the input
    if (ticks_to_wait > MAX_COUNTER_VALUE || us_wait < MIN_WAIT_US)
    {
        us_wait = MAX_COUNTER_VALUE;

#ifdef NO_MEASUREMENTS
        LOG_ERROR("Requested wait time is out of range!\r\n");
#endif
    }
    uint32_t cnt = LETIMER_CounterGet(LETIMER0);
    ticks_to_wait = ((PERIOD_TICKS + cnt - ticks_to_wait) % PERIOD_TICKS); /* request delay less than 3 secs */

    LETIMER_CompareSet(LETIMER0, 1, ticks_to_wait); // Set COMP1 value
    LETIMER_IntClear(LETIMER0, 0xFFFFFFFF);
    LETIMER_IntEnable(LETIMER0, LETIMER_IEN_COMP1); // Enable COMP1 interrupt
}

/**
 * @brief Unit test for timerWaitUs function.
 *
 * This unit test function tests the `timerWaitUs` function with different wait times including
 * small, medium, and large values. It also tests edge cases such as values smaller than the
 * minimum wait time and larger than the maximum timer value.
 *
 * @param None
 *
 * @return None
 */
void unit_test_timerWaitUs(void)
{
    // Small value (10 ms)
    gpioLed0SetOn();        // Start with LED on
    timerWaitUs_irq(10000); // Wait 10 ms

    // Medium value (100 ms)
    gpioLed0SetOff();         // Start with LED on
    timerWaitUs_irq(1000000); // Wait 1 ms

    // Large value (1 second)
    gpioLed0SetOn();         // Start with LED on
    timerWaitUs_irq(500000); // Wait 500ms

    // Medium value (100 ms)
    gpioLed0SetOff();         // Start with LED on
    timerWaitUs_irq(1000000); // Wait 1 ms

    // Large value (1 second)
    gpioLed0SetOn();         // Start with LED on
    timerWaitUs_irq(500000); // Wait 500ms

    // Test edge case - value smaller than MIN_WAIT_US
    gpioLed0SetOff();   // Start with LED on
    timerWaitUs_irq(5); // Request a wait time smaller than MIN_WAIT_US

    // Test large value - greater than max timer value
    gpioLed0SetOn();             // Start with LED on
    timerWaitUs_irq(1000000000); // Request a value greater than max ticks
#ifdef NO_MEASUREMENTS
    LOG_INFO("Completed test.");
#endif

    gpioLed0SetOff(); // Turn of led
}