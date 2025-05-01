/*
  File: scheduler.c

  Author: Samiksha Patil
  Description:
   This file (scheduler.c) contains the implementation of the scheduler functions used
   to manage and handle event flags in a real-time system. It provides functions for initializing
   the event flag storage, setting events from Interrupt Service Routines (ISRs), and retrieving
   the next pending event based on priority. T
  References:
  - Lecture Slides
*/
#include "scheduler.h"
#include <stdbool.h>
#include "em_core.h"
#define INCLUDE_LOG_DEBUG 1
#include "log.h"
#include "i2c.h"
#include "timers.h"

#include <src/lcd.h>
uint8_t const htm_service_uuid[] = {0x09, 0x18};        // Little-endian format for 0x1809 uuid
uint8_t const htm_characteristic_uuid[] = {0x1C, 0x2A}; // 0x2A1C in little-endian format

uint8_t const button_service_uuid[] = {0x89, 0x62, 0x13, 0x2D, 0x2A, 0x65, 0xEC, 0x87, 0x3E, 0x43, 0xC8, 0x38, 0x01, 0x00, 0x00, 0x00};

uint8_t const button_characteristic_uuid[] = {0x89, 0x62, 0x13, 0x2D, 0x2A, 0x65, 0xEC, 0x87, 0x3E, 0x43, 0xC8, 0x38, 0x02, 0x00, 0x00, 0x00};

// Flex Sensor Service UUID: 39b6b0dc-6ee2-4e77-a665-75d0b64649a3
static const uint8_t flexSensorService_UUID[16] = {
    0xa3, 0x49, 0x46, 0xb6, 0xd0, 0x75, 0x65, 0xa6,
    0x77, 0x4e, 0xe2, 0x6e, 0xdc, 0xb0, 0xb6, 0x39
};

// Flex Sensor Characteristic UUID: b1082442-5cb6-4d30-9d8c-12094979f6be
static const uint8_t flexSensorChar_UUID[16] = {
    0xbe, 0xf6, 0x79, 0x49, 0x09, 0x12, 0x8c, 0x9d,
    0x30, 0x4d, 0xb6, 0x5c, 0x42, 0x24, 0x08, 0xb1
};

// Accelerometer Service UUID: aa6321f1-ee79-4f7c-833f-0f6bfcdc0d32
static const uint8_t accelService_UUID[16] = {
    0x32, 0x0d, 0xdc, 0xfc, 0x6b, 0x0f, 0x3f, 0x83,
    0x7c, 0x4f, 0x79, 0xee, 0xf1, 0x21, 0x63, 0xaa
};

// Accelerometer Characteristic UUID: 5bb27a07-3455-4576-bfb6-f7ae4e45aca9
static const uint8_t accelChar_UUID[16] = {
    0xa9, 0xac, 0x45, 0x4e, 0xae, 0xf7, 0xb6, 0xbf,
    0x76, 0x45, 0x55, 0x34, 0x07, 0x7a, 0xb2, 0x5b
};

static volatile uint32_t event_flags = 0; // Event storage

typedef enum
{
  STATE_IDLE,
  STATE_STATE1,
  STATE_STATE2,
  STATE_STATE3,
  STATE_STATE4
} SensorState_t;

static SensorState_t sensorState = STATE_IDLE;
static bool initial_stage = true; // To enable button indications

typedef enum
{
  DISCOVERING_HTM_SERVICE,
  DISCOVERING_HTM_CHARACTERISTICS,
  DISCOVERING_BUTTON_SERVICE,
  DISCOVERING_BUTTON_CHARACTERISTICS,
  DISCOVERING_BUTTON_NOTIFICATION,
  WAIT_FOR_DATA
} discovery_state_t;

static discovery_state_t current_state = DISCOVERING_HTM_SERVICE;

/**
 * @brief Initializes the event flags for the scheduler.
 *
 * This function resets the event flags to zero, clearing any previously set events.
 * It is typically called during system initialization to prepare for event-based scheduling.
 *
 * @param None
 *
 * @return None
 */
void schedulerInit(void)
{

  event_flags = 0;
}

/**
 * @brief Sets the Underflow (UF) event flag for LETIMER0.
 *
 * This function is called by an Interrupt Service Routine (ISR) to indicate that an event
 * (in this case, an underflow event for LETIMER0) has occurred. It sets the corresponding
 * event flag in the global `event_flags` variable, which is later checked by the scheduler.
 *
 * @param None
 *
 * @return None
 */
void schedulerSetEventUF(void)
{
  CORE_DECLARE_IRQ_STATE;
  CORE_ENTER_CRITICAL();
  sl_bt_external_signal(EVT_LETIMER0_UF);
  CORE_EXIT_CRITICAL();
}

/**
 * @brief Sets the I2C wait event flag.
 *
 * This function sets the EVT_I2C_WAIT event flag in the global `event_flags` variable.
 * It is typically called when the system needs to wait for an I2C transaction to complete.
 *
 * @note Uses critical section to ensure atomic access to `event_flags`.
 *
 * @param None
 * @return None
 */
void schedulerSetEventI2CWait(void)
{
  CORE_DECLARE_IRQ_STATE;
  CORE_ENTER_CRITICAL();
  sl_bt_external_signal(EVT_I2C_WAIT);
  CORE_EXIT_CRITICAL();
}

/**
 * @brief Sets the Transfer Done event flag.
 *
 * This function sets the EVT_TransferDone event flag in the global `event_flags` variable.
 * It is called when an I2C data transfer is successfully completed.
 *
 * @note Uses critical section to ensure atomic access to `event_flags`.
 *
 * @param None
 * @return None
 */
void schedulerSetEventTransferDone(void)
{
  CORE_DECLARE_IRQ_STATE;
  CORE_ENTER_CRITICAL();
  sl_bt_external_signal(EVT_TransferDone);
  CORE_EXIT_CRITICAL();
}

/**
 * @brief Handles the I2C state machine for temperature sensor communication.
 *
 * This function processes external system events and transitions through various states
 * to manage the I2C-based temperature sensor. It handles power management,
 * command transmission, data reading, and BLE indications.
 *
 * The state machine operates as follows:
 * - **STATE_IDLE**: Waits for a timer event to enable sensor power and initiate a measurement delay.
 * - **STATE_STATE1**: Sends the temperature measurement command when the I2C is ready.
 * - **STATE_STATE2**: Waits for I2C transfer completion, disables IRQ, and waits for sensor conversion.
 * - **STATE_STATE3**: Reads temperature data, disables sensor power, processes temperature, and sends BLE indication.
 *
 * @param evt Pointer to the Bluetooth event structure containing external system signals.
 */
void I2CStateMachine(sl_bt_msg_t *evt)
{
  if (SL_BT_MSG_ID(evt->header) == sl_bt_evt_system_external_signal_id)
  {
    switch (sensorState)
    {

    case STATE_IDLE:
      sensorState = STATE_IDLE;
      if (evt->data.evt_system_external_signal.extsignals & EVT_LETIMER0_UF)
      {
        // Enable sensor power and initiate temperature measurement delay
        enableSensorPower();
        timerWaitUs_irq(80000);
        sensorState = STATE_STATE1;
      }
      break;

    case STATE_STATE1:
      if (evt->data.evt_system_external_signal.extsignals & EVT_I2C_WAIT)
      {
        // Send temperature measurement command
        sl_power_manager_add_em_requirement(SL_POWER_MANAGER_EM1);
        writeTemperatureCommand();
        sensorState = STATE_STATE2;
      }
      break;

    case STATE_STATE2:
      if (evt->data.evt_system_external_signal.extsignals & EVT_TransferDone)
      {
        // Disable I2C IRQ, remove power constraint, and wait for conversion
        NVIC_DisableIRQ(I2C0_IRQn);
        sl_power_manager_remove_em_requirement(SL_POWER_MANAGER_EM1);
        timerWaitUs_irq(10800); // Wait for sensor conversion (10.8 ~ 11ms delay)
        sensorState = STATE_STATE3;
      }
      break;

    case STATE_STATE3:
      if (evt->data.evt_system_external_signal.extsignals & EVT_I2C_WAIT)
      {
        // Read and process temperature data, then disable sensor
        sl_power_manager_add_em_requirement(SL_POWER_MANAGER_EM1);
        readTemperatureData();
        sensorState = STATE_STATE4;
      }
      break;

    case STATE_STATE4:
      // disableSensorPower();  //Should be enabled for LCD all the time.
      sl_power_manager_remove_em_requirement(SL_POWER_MANAGER_EM1);
      calculateAndPrintTemperature();
      sendTemperatureIndication(getBleDataPtr());
      sensorState = STATE_IDLE;
      break;
    }
  }
}

/**
 * @brief Handles Bluetooth Low Energy (BLE) discovery events.
 *
 * This function processes various Bluetooth events related to device discovery and GATT procedures.
 * It handles events such as connection opened, connection closed, GATT procedure completion,
 * service discovery, characteristic discovery, and characteristic value notifications.
 * Based on the event, it performs actions like starting service discovery, opening connections,
 * and processing received data. It also updates the display with relevant information and handles
 * errors during Bluetooth operations.
 *
 * @param evt The Bluetooth event message that contains event data and details.
 * @return None
 */
void discovery_state_machine(sl_bt_msg_t *evt)
{
  sl_status_t status;
  switch (SL_BT_MSG_ID(evt->header))
  {

  case sl_bt_evt_connection_opened_id:

    // // - 2: The length of the UUID (in this case, 2 bytes for the service UUID)
    status = sl_bt_gatt_discover_primary_services_by_uuid(getBleDataPtr()->connection_handle,
                                                          sizeof(flexSensorService_UUID),
                                                          flexSensorService_UUID);

    if (status != SL_STATUS_OK)
    {
      LOG_ERROR("Error starting primary service discovery");
    }
    current_state = DISCOVERING_HTM_SERVICE;
    // current_state = DISCOVERING_BUTTON_CHARACTERISTICS;
    break;

  case sl_bt_evt_connection_closed_id:
    current_state = DISCOVERING_HTM_SERVICE; //
    // current_state = DISCOVERING_BUTTON_CHARACTERISTICS;
    break;

  case sl_bt_evt_gatt_procedure_completed_id:
    switch (current_state)
    {
    case DISCOVERING_HTM_SERVICE:

      status = sl_bt_gatt_discover_characteristics_by_uuid(
          getBleDataPtr()->connection_handle,  // Connection handle
          getBleDataPtr()->flex_service_handle, // GATT service handle
          sizeof(flexSensorChar_UUID),     // UUID length (2 bytes)
          flexSensorChar_UUID              // Pointer to UUID
      );

      if (status != SL_STATUS_OK)
      {
        LOG_ERROR("Error discovering charc by UUID");
      }
      current_state = DISCOVERING_HTM_CHARACTERISTICS;
      break;

    case DISCOVERING_HTM_CHARACTERISTICS:

      status = sl_bt_gatt_set_characteristic_notification(evt->data.evt_gatt_characteristic.connection,
                                                          getBleDataPtr()->flex_characteristic_handle,
                                                          sl_bt_gatt_indication);
      if (status != SL_STATUS_OK)
      {
        LOG_ERROR("Error starting notifications");
      }

      current_state = DISCOVERING_BUTTON_SERVICE;
      break;

    case DISCOVERING_BUTTON_SERVICE:

      status = sl_bt_gatt_discover_primary_services_by_uuid(getBleDataPtr()->connection_handle,
                                                            sizeof(accelService_UUID),
                                                            accelService_UUID);
      if (status != SL_STATUS_OK)
      {
        LOG_ERROR("Error starting primary service discovery");
      }
      current_state = DISCOVERING_BUTTON_CHARACTERISTICS;
      break;

    case DISCOVERING_BUTTON_CHARACTERISTICS:

      status = sl_bt_gatt_discover_characteristics_by_uuid(
          getBleDataPtr()->connection_handle,     // Connection handle
          getBleDataPtr()->accel_service_handle, // GATT service handle
          sizeof(accelChar_UUID),     // UUID length (2 bytes)
          accelChar_UUID              // Pointer to UUID
      );
      if (status != SL_STATUS_OK)
      {
        LOG_ERROR("Error discovering charc by UUID");
      }

      current_state = DISCOVERING_BUTTON_NOTIFICATION;
      break;

    case DISCOVERING_BUTTON_NOTIFICATION:
      if (initial_stage)
      {
        displayPrintf(DISPLAY_ROW_CONNECTION, BLE_HANDLING_INDICATIONS);
        status = sl_bt_gatt_set_characteristic_notification(evt->data.evt_gatt_characteristic.connection,
                                                            getBleDataPtr()->accel_characteristic_handle,
                                                            sl_bt_gatt_indication);
        if (status != SL_STATUS_OK)
        {
          LOG_ERROR("Error starting notifications");
        }

        initial_stage = false;
      }
      current_state = WAIT_FOR_DATA;
      break;

    case WAIT_FOR_DATA:
      current_state = DISCOVERING_HTM_CHARACTERISTICS;
      break;

    default:
      // Handle unexpected state
      LOG_ERROR("Unexpected state");
      break;
    }
  } // end switch
}
/**
 * @brief Retrieves the next pending event.
 *
 * This function checks the event flags and returns the highest priority event (the event with
 * the lowest bit set) that is currently pending. It also clears the event flag after retrieving
 * the event, so it won't be processed again. The function ensures thread safety using critical
 * section handling to avoid race conditions.
 *
 * @param None
 *
 * @return uint32_t The next pending event, or 0 if no events are pending.
 */
uint32_t getNextEvent(void)
{
  uint32_t theEvent = 0;

  if (event_flags)
  {
    CORE_DECLARE_IRQ_STATE;
    CORE_ENTER_CRITICAL();
    // Get the highest priority event (lowest bit set)
    theEvent = event_flags; // Return all event flags
    event_flags = 0;        // Clear all events
    CORE_EXIT_CRITICAL();
  }

  return theEvent;
}
