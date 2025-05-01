#ifndef TIMERS_H
#define TIMERS_H
#include "stdint.h"

#define MAX_COUNTER_VALUE 65536

// LED timing definitions
#define LETIMER_PERIOD_MS 3000 // Period time in ms
#define ONE_MILLISECOND 1000

#define CLOCK_FREQ 1000 // ULFRCO (1 kHz) for EM3
// Compute period and on-time in timer ticks
#define PERIOD_TICKS ((LETIMER_PERIOD_MS * CLOCK_FREQ) / 1000)
#define MIN_WAIT_US      (1000000 / CLOCK_FREQ)   

// Function prototype for LETIMER initialization
void letimerInit(void);
void unit_test_timerWaitUs(void);
void timerWaitUs_polled(uint32_t us_wait);
void timerWaitUs_irq(uint32_t us_wait);

#endif // TIMERS_H
