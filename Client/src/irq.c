// irq.c

#include "irq.h"
#include "em_letimer.h"
#include "timers.h"
#include "em_gpio.h"
#include "em_core.h"
#include "app.h"
#include "gpio.h"
#include "scheduler.h"
#define INCLUDE_LOG_DEBUG 1
#include "log.h"

// LETIMER0 Interrupt Service Routine (ISR)

static uint32_t rollover_count = 0;

void LETIMER0_IRQHandler(void)
{
  // Get the enabled interrupt flags for LETIMER0
  uint32_t intFlags = LETIMER_IntGetEnabled(LETIMER0);

  // Clear the handled interrupt flags to prevent repeated triggers
  LETIMER_IntClear(LETIMER0, intFlags);

  if (intFlags & LETIMER_IF_COMP0)
  {
    schedulerSetEventUF();
    rollover_count++;
  }

  if (intFlags & LETIMER_IF_COMP1)
  {
    schedulerSetEventI2CWait();
  }
}

/**
 * @brief Computes the elapsed time in milliseconds based on LETIMER0 counter.
 *
 * This function calculates the total elapsed time in milliseconds using the current
 * LETIMER0 counter value and the rollover count. The rollover count is maintained
 * to track timer overflows, ensuring accurate time calculation over extended periods.
 *
 * @note Assumes that the `rollover_count` is correctly incremented upon each timer overflow.
 *
 * @return The total elapsed time in milliseconds.
 */
uint32_t letimerMilliseconds(void)
{
  uint32_t current_timer_count = LETIMER_CounterGet(LETIMER0);
  uint32_t total_count = current_timer_count + (rollover_count * PERIOD_TICKS);
  return ((total_count * 1000) / CLOCK_FREQ);
}

/**
 * @brief GPIO EVEN Interrupt Handler (handles PB0 press)
 */
void GPIO_EVEN_IRQHandler(void)
{
  // Clear interrupt flag
  GPIO_IntClear(1 << BUTTON0_PIN);
  CORE_DECLARE_IRQ_STATE;
  CORE_ENTER_CRITICAL();
  // Trigger external signal for the BLE stack
  sl_bt_external_signal(PB0_EXT_SIGNAL);
  CORE_EXIT_CRITICAL();
}

/**
 * @brief GPIO EVEN Interrupt Handler (handles PB1 press)
 */
void GPIO_ODD_IRQHandler(void)
{
  // Clear interrupt flag
  GPIO_IntClear(1 << BUTTON1_PIN);
  CORE_DECLARE_IRQ_STATE;
  CORE_ENTER_CRITICAL();
  // Trigger external signal for the BLE stack
  sl_bt_external_signal(PB1_EXT_SIGNAL);
  CORE_EXIT_CRITICAL();
}