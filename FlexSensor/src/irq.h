
/***********************************************************************
 * @file      irq.h
 * @version   0.1
 * @brief     Function header/interface file.
 *
 * @author    Damini Gowda, damini.gowda@colorado.edu
 * @date      Jan 27, 2023
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

#ifndef _IRQ_H_
#define _IRQ_H_

#include "gpio.h"
#include "timers.h"

void LETIMER0_IRQHandler(void);
uint32_t letimerMilliseconds(void);

#endif /* _IRQ_H_ */
