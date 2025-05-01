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
#include <src/gpio.h>
#include "app.h"
#define INCLUDE_LOG_DEBUG 1
#include "src/log.h"
#include <src/timers.h>
#include "sl_i2cspm.h"
#include "em_i2c.h"
#include <stdint.h>
#include "scheduler.h"
#include <src/lcd.h>

char tempStr[12];                         // Buffer to store the string (large enough for int range)
I2C_TransferSeq_TypeDef transferSequence; // this one can be local

#define TEMP_CONVERSION_DELAY_MS 11000 // Wait time for temp conversion (10.8ms)

uint8_t cmd_data;
uint8_t temperatureRawData[2]; // Invalid default
uint32_t rawTemperature;
float temperatureCelsius;

uint8_t htm_temperature_buffer[5]; // Buffer for the indication
uint8_t *p = &htm_temperature_buffer[0];
uint32_t htm_temperature_flt;
uint8_t flags = 0x00; // Flags byte (0x00 for Celsius)

/**
 * @brief Enable sensor power.
 *        This function powers up the sensor by turning on the relevant GPIO pin
 *        and waits for the power to stabilize.
 */
void enableSensorPower(void)
{
    gpioSensorOn();
}

/**
 * @brief Disable sensor power.
 *        This function turns off the power to the sensor by switching off the relevant GPIO pin.
 */
void disableSensorPower(void)
{
    gpioSensorOff();
}

/**
 * @brief Initialize the I2C interface.
 *        This function configures the I2C hardware, setting up the necessary pins
 *        and frequency to communicate with the sensor.
 */
void initI2C(void)
{
    // Copied from lecture slides
    //  Initialize the I2C hardware
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
    // I2C_TransferInit(I2C0, &transferSequence);
    // i2c_bus_frequency = I2C_BusFreqGet (I2C0);
}

/**
 * @brief I2C0 interrupt handler.
 *
 * This function handles I2C0 interrupts by progressing the I2C transfer state machine.
 * It checks the status of the I2C transfer and sets an event when the transfer is complete.
 * If an error occurs during the transfer, it logs the error code.
 *
 * @note This function interacts with global variables such as transferSequence, cmd_data,
 *       and read_data, which are part of the data structure passed to I2C_TransferInit().
 */
void I2C0_IRQHandler(void)
{
    // this can be locally defined
    I2C_TransferReturn_TypeDef transferStatus;
    // This shepherds the IC2 transfer along,
    // it’s a state machine! see em_i2c.c
    // It accesses global variables :
    // transferSequence
    // cmd_data
    // read_data
    // that we put into the data structure passed
    // to I2C_TransferInit()
    transferStatus = I2C_Transfer(I2C0);
    if (transferStatus == i2cTransferDone)
    {
        schedulerSetEventTransferDone();
    }
    if (transferStatus < 0)
    {
        LOG_ERROR("%d", transferStatus);
    }
} // I2C0_IRQHandler()

/**
 * @brief Perform an I2C data transfer (read or write).
 * @param dataBuffer Pointer to the data buffer
 * @param transferMode I2C_FLAG_WRITE or I2C_FLAG_READ
 * @param dataLength Number of bytes to transfer
 * @return Transfer status (0 = success, 1 = failure)
 */
uint8_t transferI2C(uint8_t *dataBuffer, uint8_t transferMode, uint8_t dataLength)
{

    // Copied from lecture slides
    I2C_TransferReturn_TypeDef transferStatus; // make this global for IRQs in A4

    transferSequence.addr = SI7021_DEVICE_ADDR << 1; // shift device address left
    transferSequence.flags = transferMode;
    transferSequence.buf[0].data = dataBuffer; // pointer to data to write
    transferSequence.buf[0].len = dataLength;

    NVIC_EnableIRQ(I2C0_IRQn);

    transferStatus = I2C_TransferInit(I2C0, &transferSequence);
    if (transferStatus < 0)
    {
        LOG_ERROR("I2C_TransferInit() Write error = %d", transferStatus);
    }
    return 0;
}

/**
 * @brief Write the temperature measurement command to the sensor.
 *        This function sends the temperature measurement command to the SI7021 sensor
 *        via I2C to trigger a temperature conversion.
 */
void writeTemperatureCommand(void)
{
    cmd_data = SI7021_TEMP_MEASURE_CMD;
    transferI2C(&cmd_data, I2C_FLAG_WRITE, sizeof(cmd_data));
}

/**
 * @brief Read the temperature data from the sensor.
 *        This function reads the raw temperature data from the SI7021 sensor
 *        using I2C and stores it in the global `temperatureRawData` array.
 */
void readTemperatureData(void)
{
    // LOG_INFO("Before processing: temperatureRawData = 0x%d", temperatureRawData);
    transferI2C(temperatureRawData, I2C_FLAG_READ, sizeof(temperatureRawData));
}

/**
 * @brief Converts an integer to a string with a "TEMP=" prefix.
 *
 * This function takes an integer, converts it to a string, and appends it to
 * the prefix "TEMP=". It correctly handles negative numbers and ensures
 * proper ordering of digits.
 *
 * @param num The integer value to be converted.
 * @param str Pointer to the character array where the resulting string is stored.
 *            The caller must ensure that the array is large enough to hold
 *            "TEMP=" followed by the number and a null terminator.
 */
void intToStrWithTemp(int num, char *str)
{
    // Add "TEMP=" prefix
    char prefix[] = "TEMP=";
    int i = 0;

    // Copy the prefix into str
    while (prefix[i] != '\0')
    {
        str[i] = prefix[i];
        i++;
    }

    if (num < 0)
    {
        str[i++] = '-'; // Store the negative sign first
        num = -num;     // Convert to positive for processing
    }

    int digitStartIdx = i; // Where the number actually starts

    // Convert each digit to char (stored in reverse order)
    do
    {
        str[i++] = (num % 10) + '0';
        num /= 10;
    } while (num > 0);

    str[i] = '\0'; // Null-terminate the string

    // Reverse only the number part (excluding "TEMP=" and negative sign if present)
    for (int j = digitStartIdx, k = i - 1; j < k; j++, k--)
    {
        char temp = str[j];
        str[j] = str[k];
        str[k] = temp;
    }
}

/**
 * @brief Convert raw temperature data and print it in Celsius.
 *        This function takes the raw temperature data from the sensor, converts it
 *        to Celsius using the SI7021 conversion formula, and logs the result.
 */
void calculateAndPrintTemperature(void)
{

    rawTemperature = (temperatureRawData[0] << 8) | temperatureRawData[1];
    temperatureCelsius = ((175.72 * rawTemperature) / 65536.0) - 46.85;
#ifdef NO_MEASUREMENTS
    // LOG_INFO("Temperature: %.2f C\n", temperatureCelsius); //Commented since no coments are required for this assignment
    // LOG_INFO("Millisecond value is %u",loggerGetTimestamp()); //commenting this out since we are not using it
#endif
}

/**
 * @brief Read and print temperature from SI7021 sensor.
 *        This function enables the sensor power, triggers the temperature measurement,
 *        waits for the conversion to complete, reads the data, and prints the temperature.
 */
void measureAndPrintTemperature(void)
{
    enableSensorPower();
    writeTemperatureCommand();
    timerWaitUs_irq(TEMP_CONVERSION_DELAY_MS); // Wait for sensor conversion (10.8 ~ 11ms delay)
    readTemperatureData();
    calculateAndPrintTemperature();
    disableSensorPower();
}

/**
 * @brief Sends a temperature measurement indication over BLE.
 *
 * This function constructs and sends a GATT indication for the Health Thermometer Service.
 * It ensures that indications are only sent when:
 * - A BLE connection is active.
 * - The client has enabled indications.
 * - No previous indication is still in progress.
 *
 * The function formats the temperature value into IEEE-11073 32-bit float format
 * before sending it as a GATT indication.
 *
 * @param bleDataPtr Pointer to the BLE data structure containing connection information.
 */
void sendTemperatureIndication(ble_data_struct_t *bleDataPtr)
{
    sl_status_t sc = sl_bt_gatt_server_write_attribute_value(
        gattdb_temperature_measurement, // Attribute handle
        0,                              // Offset
        sizeof(htm_temperature_buffer),
        htm_temperature_buffer); // Data buffer

    if (sc != SL_STATUS_OK)
    {
        LOG_ERROR("GATT write attribute failed, error");
    }

    if (!bleDataPtr->connection_open)
    {
        // For debugging purpose
        //  LOG_WARN("No active connection, skipping indication.");
        return;
    }

    if (!bleDataPtr->ok_to_send_htm_indications)
    {
        // For debugging purpose
        //  LOG_WARN("Client has not enabled indications.");
        return;
    }

    uint8_t htm_temperature_buffer[5];
    uint8_t *p = &htm_temperature_buffer[0];
    uint32_t htm_temperature_flt;
    uint8_t flags = 0x00;

    UINT8_TO_BITSTREAM(p, flags);
    htm_temperature_flt = INT32_TO_FLOAT((int32_t)(temperatureCelsius * 1000), -3);
    UINT32_TO_BITSTREAM(p, htm_temperature_flt);

    // If indications are ENABLED, while checking the connection open and bonding, send BLE Indication
    if (bleDataPtr->ok_to_send_htm_indications == true && bleDataPtr->connection_open == true)
    {
        // Check if we can send immediately send or need to enqueue
        if (bleDataPtr->indication_in_flight == false && (get_queue_depth() == 0))
        {
            // Send GATT indication
            sc = sl_bt_gatt_server_send_indication(
                bleDataPtr->connection_handle,  // Use stored connection handle
                gattdb_temperature_measurement, // Handle from gatt_db.h
                sizeof(htm_temperature_buffer),
                htm_temperature_buffer);

            if (sc != SL_STATUS_OK)
            {
                LOG_ERROR("GATT Indication Failed, error");
                write_queue(
                    gattdb_temperature_measurement, // Handle from gatt_db.h
                    sizeof(htm_temperature_buffer),
                    htm_temperature_buffer);
            }
            else
            {
                // LOG_INFO("GATT Indication Sent.");
                bleDataPtr->indication_in_flight = true; // Mark indication as in-flight
            }
        }
        else
        { // If in flight is true or if the queue has few elements, send it to queue
            write_queue(
                gattdb_temperature_measurement,
                sizeof(htm_temperature_buffer),
                htm_temperature_buffer);
        }
    }

    intToStrWithTemp((int)temperatureCelsius, tempStr); // Convert the int to string
    displayPrintf(DISPLAY_ROW_TEMPVALUE, tempStr);      // Print the Temp value on LCD with "TEMP=" prefi
}

/**
 * @brief Reset and clean up I2C peripheral.
 *        This function resets the I2C peripheral, disables I2C, and turns off the
 *        relevant GPIO pins to ensure that the I2C interface is properly cleaned up.
 */
void cleanupI2C(void)
{

    gpioI2cSclOff();
    gpioI2cSdaOff();

    gpioSensorOff();
}
