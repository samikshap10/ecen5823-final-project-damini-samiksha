/***********************************************************************
 * @file      adc.h
 * @version   0.1
 * @brief     Function header/interface file.
 *
 * @author    Damini Gowda, damini.gowda@colorado.edu
 * @date      April 10, 2025
 *
 *
 * @institution University of Colorado Boulder (UCB)
 * @course      ECEN 5823-001: IoT Embedded Firmware
 * @instructor  Chris Choi
 */

#ifndef SRC_ADC_H_
#define SRC_ADC_H_

#include <stdio.h>
#include <stdlib.h>
#include "em_device.h"
#include "em_chip.h"
#include "em_cmu.h"
#include "em_adc.h"
#include "src/scheduler.h"

void initADC (void);

#endif /* SRC_ADC_H_ */
