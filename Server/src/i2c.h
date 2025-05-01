#ifndef I2C_H
#define I2C_H
#include <stdint.h>
// SI7021 I2C Address and Commands
#define SI7021_DEVICE_ADDR 0x40
#define SI7021_TEMP_MEASURE_CMD 0xF3

// Function Prototypes
// Sensor enable and disbale functions
void enableSensorPower(void);
void disableSensorPower(void);
// I2C Init function
void initI2C(void);
// I2C Trnasfer Function
uint8_t transferI2C(uint8_t *dataBuffer, uint8_t transferMode, uint8_t dataLength);
// Write temperature command function
void writeTemperatureCommand(void);
// Read temperature command function
void readTemperatureData(void);
// Calculate and Print the Temparture Function
void calculateAndPrintTemperature(void);
// I2C Cleanup Function
void cleanupI2C(void);
// Measure and print temperature
void measureAndPrintTemperature(void);
// Wait for converstion function
void waitForConversion(void);
void reg_bank_sel(uint8_t bank);
void who_am_i(void);
uint8_t readRegister(uint8_t regAddr);
void set_acc_sensor(void);
void clear_interrupt_flag(void);
#endif // I2C_H
