
/***********************************************************************
 * @file      timers.h
 * @version   0.1
 * @brief     Function header/interface file.
 *
 * @author    Damini Gowda, damini.gowda@colorado.edu
 * @date      Jan 26, 2023
 *
 *
 * @institution University of Colorado Boulder (UCB)
 * @course      ECEN 5823-001: IoT Embedded Firmware
 * @instructor  Chris Choi
 *
 * @assignment Assignment 2 - Managing Energy Mode
 * @due
 *
 * @resources None
 *
 */

#ifndef _TIMERS_H_
#define _TIMERS_H_

#include <stdint.h>
#include "src/scheduler.h"

void LETIMER0Init(void);
void LETIMER0EnableIrq(void);
void timerWaitUs_poll(uint32_t us_wait);
void timerWaitUs_irq(uint32_t us_wait);
void timerWaitUs_poll_UnitTest(void);
//void timerWaitUs_irq_UnitTest(Events_t evt);

#endif /*_TIMERS_H_ */
