/***********************************************************************
 * @file      i2c.c
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

#include "src/i2c.h"

#define INCLUDE_LOG_DEBUG   1
#include "src/log.h"

#define SI7021_I2CADDR 0x40
#define SI7021_CMD_DATA   0xF3

uint8_t cmd_data = SI7021_CMD_DATA;
uint8_t tempData[2];
I2C_TransferSeq_TypeDef transferSeq;
I2C_TransferReturn_TypeDef transferStatus;
static uint32_t Calc_Temp;
/**
 * @brief Initializes I2C0 with the specified configuration.
 *
 * This function configures the I2C0 peripheral for communication, setting the
 * appropriate SCL and SDA pins, and initializing the I2C with a standard clock
 * rate and standard clock low/high ratio.
 */
void I2C0_init(void){

  I2CSPM_Init_TypeDef i2c_config_struct = {
  .port = I2C0,
  .sclPort = gpioPortC,
  .sclPin = 10,
  .sdaPort = gpioPortC,
  .sdaPin = 11,
  .portLocationScl = 14,
  .portLocationSda = 16,
  .i2cRefFreq = 0,
  .i2cMaxFreq = I2C_FREQ_STANDARD_MAX,
  .i2cClhr = i2cClockHLRStandard
  };

  //Initialize the I2C
  I2CSPM_Init(&i2c_config_struct);

  toggleSCL();

  NVIC_EnableIRQ(I2C0_IRQn);
}

/**
 * @brief Performs an I2C transfer (either read or write).
 *
 * This function executes an I2C transfer, either reading from or writing to the
 * specified device. It takes care of setting up the transfer sequence and
 * checking for errors during the transfer process.
 *
 * @param Data Pointer to the data buffer to be transferred.
 * @param Read_Write Specifies whether the transfer is a read or write operation.
 * @param DataLen Length of the data buffer to be transferred.
 *
 * @return 0 if the transfer was successful, 1 if an error occurred.
 */
uint8_t I2CTransfer(uint8_t* Data, uint8_t Read_Write, uint8_t DataLen)
{
  I2C_TransferReturn_TypeDef transferStatus;
  I2C_TransferSeq_TypeDef transferSeq;

  //I2C0_init();

  /* Fill the transfer sequence structure */
  transferSeq.addr    = (SI7021_I2CADDR << 1);
  transferSeq.flags   = Read_Write;
  transferSeq.buf[0].data  = Data;
  transferSeq.buf[0].len   = DataLen;

  //Enable NVIC for I2C0
  NVIC_EnableIRQ(I2C0_IRQn);

  /* Initialize a transfer */
  transferStatus = I2C_TransferInit(I2C0, &transferSeq);

  /* LOG an error if transfer failed */
  if(transferStatus != i2cTransferDone)
    {
      LOG_ERROR("I2CSPM_Transfer Failed. Error = %d\n\r", transferStatus);
      return 1;
    }

  return 0;
}

/**
 * @brief Powers on the Si7021 sensor.
 *
 * This function powers on the Si7021 temperature and humidity sensor and waits
 * for the required time to ensure it is ready for operation.
 */
void powerOnSi7021(void)
{
  // Turn on power to the sensor and wait for 80ms
  gpioSi7021Enable();
  timerWaitUs_irq(80000);
}

/**
 * @brief Powers off the Si7021 sensor.
 *
 * This function powers off the Si7021 temperature and humidity sensor.
 */
void powerOffSi7021(void)
{
  gpioSi7021Disable();
 // timerWaitUs_poll(80000);
}

/**
 * @brief Waits for the conversion time of the Si7021 sensor.
 *
 * This function pauses execution for the amount of time required by the
 * Si7021 sensor to complete a measurement and have the data ready for reading.
 */
void conversionWaitTimeSi7021(void)
{
  // Wait for 11ms for the sensor to get the data
  timerWaitUs_irq(10800);
}

/**
 * @brief Writes data to the Si7021 sensor via I2C.
 *
 * This function sends a command to the Si7021 sensor to initiate a temperature
 * reading. It checks for any errors that occur during the write operation.
 */
void I2CWriteData(void)
{
  uint8_t ret;

  // Send the Temperature read command and check for error
  ret = I2CTransfer(&cmd_data, I2C_FLAG_WRITE, sizeof(cmd_data));
  if(ret == 1)
  {
      //LOG_ERROR("I2CSPM_Transfer write Failed. Data = %d\n\r", cmd_data);
  }
}

/**
 * @brief Reads data from the Si7021 sensor via I2C.
 *
 * This function reads the temperature data from the Si7021 sensor. It checks
 * for any errors that occur during the read operation.
 */
void I2CReadData(void)
{
  uint8_t ret;

  // Receive the temperature data and check for error
  ret = I2CTransfer(tempData, I2C_FLAG_READ, sizeof(tempData));
  if(ret == 1)
  {
     // LOG_ERROR("I2CSPM_Transfer Read Failed. Data = %d\n\r", ((tempData[0] << 8) | tempData[1]));
  }
}

/**
 * @brief Calculates and logs the temperature from the Si7021 sensor.
 *
 * This function reads the temperature data, processes it, and converts the
 * value into degrees Celsius. The calculated temperature is then logged.
 */
void printTemperatureSi7021(void)
{
  //float Calc_Temp;
  uint16_t Read_Temp;

  // Arrange MSB and LSB into Read_Temp variable
  Read_Temp = ((tempData[0] << 8) | tempData[1]);

  // Convert received value to degrees Celsius and log
  Calc_Temp = (175.72 * Read_Temp)/65536 - 46.85;

  //LOG_INFO("Temperature in C = %d\n\r", Calc_Temp);

  displayPrintf(DISPLAY_ROW_TEMPVALUE, "Temperature=%d", Calc_Temp);

}

uint32_t* returnTemp(void){
  return &Calc_Temp;
}
/**
 * @brief Teardown and cleanup of the I2C and Si7021 sensor.
 *
 * This function resets the I2C peripheral, disables the I2C clock, and disables
 * the GPIO pins used for I2C communication. It also powers down the Si7021 sensor.
 */
void I2CTeardown(void){

  // reset and disable I2C clock module
  I2C_Reset(I2C0);
  I2C_Enable(I2C0, false);

  // disable SCL and SDA GPIOs
  gpioI2CSDADisable();
  gpioI2CSCLDisable();

  // Disable Si7021 - Turn Power Off
  gpioSi7021Disable();
}

void toggleSCL(void){

  powerOnSi7021();

  for(int i=0; i<5; i++){
      gpioI2CSCLToggle();
      timerWaitUs_poll(10000);
  }

  powerOffSi7021();
}


/**
 * @brief Performs a full I2C communication test with the Si7021 sensor.
 *
 * This function executes a series of steps to test the I2C communication with
 * the Si7021 sensor. It powers on the sensor, writes a command to initiate
 * a temperature reading, waits for the sensor to process the data, reads the
 * temperature data from the sensor, prints the temperature, and finally powers
 * off the sensor.
 */
void readTemperature(void){

  //Turn on power to the Si7021
  powerOnSi7021();
  //Take temperature reading from Si7021
  I2CTransferWrite();
  conversionWaitTimeSi7021();
  I2CTransferRead();
  //Turn off power to the Si7021
  powerOffSi7021();
  printTemperatureSi7021();

}

/**
 * @brief Writes data to the Si7021 sensor via I2C.
 *
 * This function sends a command to the Si7021 sensor to initiate a temperature
 * reading. It checks for any errors that occur during the write operation.
 */
void I2CTransferWrite(void)
{
  //I2C_TransferReturn_TypeDef transferStatus;

  //I2C0_init();

  /* Fill the transfer sequence structure */
  transferSeq.addr    = (SI7021_I2CADDR << 1);
  transferSeq.flags   = I2C_FLAG_WRITE;
  transferSeq.buf[0].data  = &cmd_data;
  transferSeq.buf[0].len   = sizeof(cmd_data);

  //Enable NVIC for I2C0
  NVIC_EnableIRQ(I2C0_IRQn);

  /* Initialize a transfer */
  transferStatus = I2C_TransferInit(I2C0, &transferSeq);

  /* LOG an error if transfer failed */
  if(transferStatus < 0)
    {
      LOG_ERROR("I2C_TransferInit write Failed. Error = %d\n\r", transferStatus);
    }
}

/**
 * @brief Reads data from the Si7021 sensor via I2C.
 *
 * This function reads the temperature data from the Si7021 sensor. It checks
 * for any errors that occur during the read operation.
 */
void I2CTransferRead(void)
{
  //I2C_TransferReturn_TypeDef transferStatus;

  //I2C0_init();

  /* Fill the transfer sequence structure */
  transferSeq.addr    = (SI7021_I2CADDR << 1);
  transferSeq.flags   = I2C_FLAG_READ;
  transferSeq.buf[0].data  = tempData;
  transferSeq.buf[0].len   = sizeof(tempData);

  //Enable NVIC for I2C0
  NVIC_EnableIRQ(I2C0_IRQn);

  /* Initialize a transfer */
  transferStatus = I2C_TransferInit(I2C0, &transferSeq);

  /* LOG an error if transfer failed */
  if(transferStatus < 0)
    {
      LOG_ERROR("I2C_TransferInit read Failed. Error = %d\n\r", transferStatus);
    }
}

/**
 * @brief Interrupt Service Routine (ISR) for the I2C0 interrupt.
 *
 * This function is triggered when an interrupt occurs on the I2C0 peripheral.
 * It handles the completion of the I2C transfer by checking the status and
 * setting the appropriate event flag if the transfer is completed successfully.
 * In case of an error during the I2C transfer, it logs the error.
 *
 * @note This function should be executed in response to an I2C0 interrupt and is
 *       typically used to handle I2C communication events like transfer completion
 *       or errors in communication.
 *
 * @retval None
 */
void I2C0_IRQHandler(void){

  //I2C_TransferReturn_TypeDef transferStatus;

  transferStatus = I2C_Transfer(I2C0);

  if(transferStatus == i2cTransferDone){
      schedulerSetEventI2CDone();
  }

  if(transferStatus < 0){
      LOG_ERROR("I2C_Transfer: error = %d\r\n", transferStatus);
  }

}// I2C0_IRQHandler()

