/*
  File: i2c_temperature.c

  Author: Samiksha Patil
  Description:
   This file (`i2c_temperature.c`) contains the implementation for interfacing with the SI7021 temperature sensor via I2C.
   The file includes functions for initializing I2C, reading temperature data, and converting raw data into Celsius.
   It also provides functionality to manage the sensor's power and cleanup the I2C peripheral.

  References:
  - Lecture Slides
*/

#include "i2c.h"
#include "app.h"
#include <src/gpio.h>
#define INCLUDE_LOG_DEBUG 1
#include "src/log.h"
#include <src/timers.h>
#include "sl_i2cspm.h"
#include "em_i2c.h"
#include <stdint.h>
#include "scheduler.h"

#define ICM20948_ADDR 0x69         // Replace if AD0 = HIGH
#define ICM20948_WHO_AM_I_REG 0x00 // Example register to read
#define I2C_TRANSFER_TIMEOUT 10000

#define TEMP_CONVERSION_DELAY_MS 11000 // Wait time for temp conversion (10.8ms)

/**
 * @brief Initialize the I2C interface for ICM-20948.
 */
void initI2C(void)
{
    I2CSPM_Init_TypeDef I2C_Config = {
        .port = I2C0,
        .sclPort = gpioPortC,
        .sclPin = 10,
        .sdaPort = gpioPortC,
        .sdaPin = 11,
        .portLocationScl = 14,
        .portLocationSda = 16,
        .i2cRefFreq = 0,
        .i2cMaxFreq = I2C_FREQ_STANDARD_MAX,
        .i2cClhr = i2cClockHLRStandard};
    I2CSPM_Init(&I2C_Config);
}

/**
 * @brief Perform an I2C data transfer (read or write).
 * @param dataBuffer Pointer to the data buffer
 * @param transferMode I2C_FLAG_WRITE or I2C_FLAG_READ
 * @param dataLength Number of bytes to transfer
 * @return Transfer status (0 = success, 1 = failure)
//  */
uint8_t transferI2C(uint8_t *dataBuffer, uint8_t transferMode, uint8_t dataLength)
{

    // Copied from lecture slides
    I2C_TransferReturn_TypeDef transferStatus; // make this global for IRQs in A4
    I2C_TransferSeq_TypeDef transferSequence;  // this one can be local

    transferSequence.addr = ICM20948_ADDR << 1; // shift device address left
    transferSequence.flags = transferMode;
    transferSequence.buf[0].data = dataBuffer; // pointer to data to write
    transferSequence.buf[0].len = dataLength;

    transferStatus = I2CSPM_Transfer(I2C0, &transferSequence);
    if (transferStatus != i2cTransferDone)
    {
        LOG_ERROR("I2CSPM_Transfer: I2C bus write of cmd=0x%02X with status code: %d", dataBuffer[0], transferStatus);
        return 1;
    }
    return 0;
}

uint8_t readRegister(uint8_t regAddr)
{
    uint8_t data;
    I2C_TransferSeq_TypeDef transfer;
    I2C_TransferReturn_TypeDef result;

    transfer.addr = ICM20948_ADDR << 1;
    transfer.flags = I2C_FLAG_WRITE_READ;

    transfer.buf[0].data = &regAddr;
    transfer.buf[0].len = 1;

    transfer.buf[1].data = &data;
    transfer.buf[1].len = 1;

    result = I2CSPM_Transfer(I2C0, &transfer);
    if (result != i2cTransferDone)
    {
        LOG_ERROR("Read failed for reg 0x%02X, error %d", regAddr, result);
        return 0xFF;
    }

    return data;
}

void reg_bank_sel(uint8_t bank)
{
    uint8_t writeData[4];
    writeData[0] = 0x7F;      // Bank select register
    writeData[1] = bank << 4; // Bank number shifted into bits [5:4]
    transferI2C(writeData, I2C_FLAG_WRITE, 2);
}

void enable_interrupt(void)
{
    reg_bank_sel(0);
    // Example: Enable Wake-on-Motion interrupt
    uint8_t int_enable[2] = {0x10, 0x08}; // 0x08 = 0b00001000 (WOM_INT_EN)
    transferI2C(int_enable, I2C_FLAG_WRITE, 2);
}

void select_WOM_algorithm(void)
{
    reg_bank_sel(2);
    // Example: Enable Wake-on-Motion interrupt
    uint8_t wom_algo[2] = {0x12, 0x01}; // 0x08 = 0b00001000 (WOM_INT_EN)
    transferI2C(wom_algo, I2C_FLAG_WRITE, 2);
}

void WOM_Threshold(void)
{
    reg_bank_sel(2);
    uint8_t wom_thr[2] = {0x13, 0x14};
    transferI2C(wom_thr, I2C_FLAG_WRITE, 2); // while Bank 2 is selected
}

void LP_Config(void)
{
    reg_bank_sel(0);
    uint8_t ip_config[2] = {0x05, 0x00};
    transferI2C(ip_config, I2C_FLAG_WRITE, 2); // while Bank 2 is selected
}

void accel_intel_config(void)
{
    reg_bank_sel(2);
    uint8_t accel_intel_ctrl[2] = {0x12, 0x03}; // 0b00000011 → Enable WOM + mode = compare to previous
    transferI2C(accel_intel_ctrl, I2C_FLAG_WRITE, 2);
}

void int_pin_config(void)
{
    reg_bank_sel(0);
    uint8_t int_pin_cfg[2] = {0x0F, 0x00}; // 0b00100000
    transferI2C(int_pin_cfg, I2C_FLAG_WRITE, 2);
}

void power_mng(void)
{
    reg_bank_sel(0);
    uint8_t ip_config[2] = {0x06, 0x29};
    transferI2C(ip_config, I2C_FLAG_WRITE, 2); // while Bank 2 is selected

    uint8_t pwr_mgmt_2[2] = {0x07, 0x07}; // 0b00000111
    transferI2C(pwr_mgmt_2, I2C_FLAG_WRITE, 2);
}

void set_acc_sensor(void)
{
    who_am_i();
    //    select_WOM_algorithm();
    LOG_INFO("WOM Algorithm set\n");

    WOM_Threshold();

    accel_intel_config();
    LOG_INFO("WoM logic and sets the comparison mode\n");

    int_pin_config();

    enable_interrupt();

    LP_Config();

    power_mng();

    LOG_INFO("Set done\n");
}
/**
 * @brief Enable sensor power.
 *        This function selects Bank 0 by writing 0x00 to register 0x7F.
 */
void enableSensorPower(void)
{
    who_am_i();
    reg_bank_sel(0);
    LOG_INFO("SELECTED BANK");

    uint8_t wakeCmd[2] = {0x06, 0x81}; // PWR_MGMT_1 = 0x01 (wake up, clock source auto)
    transferI2C(wakeCmd, I2C_FLAG_WRITE, 2);

    // Enable the accelerometer and gyroscope (PWR_MGMT_2 = 0x00)
    uint8_t enableSensors[2] = {0x07, 0x00};
    transferI2C(enableSensors, I2C_FLAG_WRITE, 2);
}

void who_am_i(void)
{
    uint8_t whoami = readRegister(0x00); // WHO_AM_I
    LOG_INFO("WHO_AM_I: 0x%02X", whoami);
}
void clear_interrupt_flag(void)
{
    reg_bank_sel(0);
    uint8_t int_status_reg[1] = {0x19};
    uint8_t int_status_value[1] = {0x00};

    transferI2C(int_status_reg, I2C_FLAG_WRITE, 1);
    transferI2C(int_status_value, I2C_FLAG_READ, 1);

    // Optionally check if bit 3 is set:
    if (int_status_value[0] & 0x08)
    {
        LOG_INFO("Wake-on-Motion interrupt detected and cleared.\n");
    }
}
/**
 * @brief Disable sensor power.
 *        This function wakes up the sensor by writing 0x01 to register 0x06 (PWR_MGMT_1).
 */
void disableSensorPower(void)
{
    // yet to write this function
}
