/***********************************************************************
 * @file      i2c.h
 * @version   0.1
 * @brief     Function header/interface file.
 *
 * @author    Damini Gowda, damini.gowda@colorado.edu
 * @date      Feb 2, 2025
 *
 *
 * @institution University of Colorado Boulder (UCB)
 * @course      ECEN 5823-001: IoT Embedded Firmware
 * @instructor  Chris Choi
 *
 * @assignment Assignment 3 - Si7021 and Load Power Management Part 1
 * @due
 *
 * @resources EFR32xG13 Wireless GeckoReference Manual, Lecture PDFs, AN and reading references
 *
 */

#ifndef SRC_I2C_H_
#define SRC_I2C_H_

#include "sl_i2cspm.h"
#include "src/gpio.h"
#include "src/timers.h"

void I2C0_init(void);
uint8_t I2CTransfer(uint8_t* Data, uint8_t Read_Write, uint8_t DataLen);
void powerOnSi7021(void);
void powerOffSi7021(void);
void conversionWaitTimeSi7021(void);
void I2CWriteData(void);
void I2CReadData(void);
void printTemperatureSi7021(void);
void I2CTeardown(void);
void readTemperature(void);
void I2CTransferWrite(void);
void I2CTransferRead(void);
uint32_t* returnTemp(void);
void toggleSCL(void);

#endif /* SRC_I2C_H_ */
