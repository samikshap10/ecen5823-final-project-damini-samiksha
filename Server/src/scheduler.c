/***********************************************************************
 * @file      scheduler.c
 * @version   0.1
 * @brief     Function header/interface file.
 *
 * @author    Damini Gowda, damini.gowda@colorado.edu
 * @date      Feb 1, 2025
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

#define INCLUDE_LOG_DEBUG   1
#include "src/scheduler.h"

static volatile Events_t event_flags = EVENT_NONE;  // Bit-field to track events
static volatile uint8_t counter3s =0;
extern uint8_t flag;
/**
 * @brief Sets the LETIMER0 underflow event in the scheduler event flags.
 *
 * This function is called from the interrupt service routine (ISR) for the LETIMER0
 * underflow event to mark that the event has occurred, allowing the main application
 * to process it during the next loop.
 *
 * @note This function modifies the global event flags in a critical section to
 *       prevent race conditions when accessing shared data.
 */
void schedulerSetEventUF(void) {

  sl_status_t rc = SL_STATUS_OK;

  CORE_DECLARE_IRQ_STATE;

  CORE_ENTER_CRITICAL();

  //event_flags |= EVENT_LETIMER_UF;
  rc = sl_bt_external_signal(EVENT_LETIMER_UF);

  if(rc != SL_STATUS_OK)
    {
    //  LOG_ERROR("sl_bt_external_signal failed:EVENT_LETIMER_UF\n\r");
    }

  CORE_EXIT_CRITICAL();
}

/**
 * @brief Sets the LETIMER COMP1 event in the scheduler event flags.
 *
 * This function is called from the interrupt service routine (ISR) for the LETIMER0
 * COMP1 event to mark that the event has occurred, allowing the main application
 * to process it during the next loop.
 *
 * @note This function modifies the global event flags in a critical section to
 *       prevent race conditions when accessing shared data.
 */
void schedulerSetEventCOMP1(void) {

  sl_status_t rc = SL_STATUS_OK;

  CORE_DECLARE_IRQ_STATE;

  CORE_ENTER_CRITICAL();

  //event_flags |= EVENT_LETIMER_COMP1;
  rc =  sl_bt_external_signal(EVENT_LETIMER_COMP1);

  if(rc != SL_STATUS_OK)
    {
   //   LOG_ERROR("sl_bt_external_signal failed:EVENT_LETIMER_COMP1\n\r");
    }

  CORE_EXIT_CRITICAL();
}

/**
 * @brief Sets the I2C transfer done event in the scheduler event flags.
 *
 * This function is called from the interrupt service routine (ISR) for the I2C
 * transfer done event to mark that the event has occurred, allowing the main application
 * to process it during the next loop.
 *
 * @note This function modifies the global event flags in a critical section to
 *       prevent race conditions when accessing shared data.
 */
void schedulerSetEventI2CDone(void) {

  sl_status_t rc = SL_STATUS_OK;

  CORE_DECLARE_IRQ_STATE;

  CORE_ENTER_CRITICAL();

  //event_flags |= EVENT_I2CTransfer_Done;
  rc = sl_bt_external_signal(EVENT_I2CTransfer_Done);

  if(rc != SL_STATUS_OK)
    {
  //    LOG_ERROR("sl_bt_external_signal failed:EVENT_I2CTransfer_Done\n\r");
    }

  CORE_EXIT_CRITICAL();
}

/**
 * @brief Signals that the BLE connection has been closed.
 *
 * This function sends an external signal to indicate that the BLE connection
 * has been closed. It ensures atomic access using critical section protection.
 *
 * @param None
 * @return None
 */
void schedulerSetEventBleConnectionClose(void) {

  sl_status_t rc = SL_STATUS_OK;

  CORE_DECLARE_IRQ_STATE;

  CORE_ENTER_CRITICAL();

  rc =  sl_bt_external_signal(EVENT_BLEConnectionClose);

  if(rc != SL_STATUS_OK)
    {
//      LOG_ERROR("sl_bt_external_signal failed:EVENT_BLEConnectionClose\n\r");
    }

  CORE_EXIT_CRITICAL();
}

/**
 * @brief Sets the event for PB0 press.
 *
 * This function generates an external signal for the PB0 button press event
 * and reports any errors if the signal fails to be set.
 *
 * It enters a critical section to prevent race conditions while setting the signal.
 */
void schedulerSetEventPB0(void) {

  sl_status_t rc = SL_STATUS_OK;

  CORE_DECLARE_IRQ_STATE;

  CORE_ENTER_CRITICAL();

  rc =  sl_bt_external_signal(EVENT_PB0);

  if(rc != SL_STATUS_OK)
    {
 //     LOG_ERROR("sl_bt_external_signal failed:EVENT_PB0\n\r");
    }

  CORE_EXIT_CRITICAL();
}

void schedulerSetEvent0(void){

  sl_status_t rc = SL_STATUS_OK;

  CORE_DECLARE_IRQ_STATE;

  CORE_ENTER_CRITICAL();

  rc =  sl_bt_external_signal(EVENT_0DEGREE);

  if(rc != SL_STATUS_OK)
    {
 //     LOG_ERROR("sl_bt_external_signal failed:EVENT_0DEGREE\n\r");
    }

  CORE_EXIT_CRITICAL();
}

void schedulerSetEvent45(void){

  sl_status_t rc = SL_STATUS_OK;

  CORE_DECLARE_IRQ_STATE;

  CORE_ENTER_CRITICAL();

  rc =  sl_bt_external_signal(EVENT_45DEGREE);

  if(rc != SL_STATUS_OK)
    {
  //    LOG_ERROR("sl_bt_external_signal failed:EVENT_0DEGREE\n\r");
    }

  CORE_EXIT_CRITICAL();
}

void schedulerSetEvent90(void){

  sl_status_t rc = SL_STATUS_OK;

  CORE_DECLARE_IRQ_STATE;

  CORE_ENTER_CRITICAL();

  rc =  sl_bt_external_signal(EVENT_90DEGREE);

  if(rc != SL_STATUS_OK)
    {
//      LOG_ERROR("sl_bt_external_signal failed:EVENT_0DEGREE\n\r");
    }

  CORE_EXIT_CRITICAL();
}

void schedulerSetEventAccelINT(void) {

  sl_status_t rc = SL_STATUS_OK;

  CORE_DECLARE_IRQ_STATE;

  CORE_ENTER_CRITICAL();

  rc =  sl_bt_external_signal(EVENT_ACCELINT);

  if(rc != SL_STATUS_OK)
    {
 //     LOG_ERROR("sl_bt_external_signal failed:EVENT_ACCELINT\n\r");
    }

  CORE_EXIT_CRITICAL();
}

void schedulerSetEventBLEDONE(void) {

  sl_status_t rc = SL_STATUS_OK;

  CORE_DECLARE_IRQ_STATE;

  CORE_ENTER_CRITICAL();

  rc =  sl_bt_external_signal(EVENT_BLEDONE);

  if(rc != SL_STATUS_OK)
    {
      LOG_ERROR("sl_bt_external_signal failed:EVENT_BLEDONE\n\r");
    }

  CORE_EXIT_CRITICAL();
}
/**
 * @brief Retrieves the next event to be processed and clears the event flags.
 *
 * This function is used by the main application to fetch the next event that
 * has been set (e.g., LETIMER0 underflow event) and clears the event flag for
 * future use.
 *
 * @return uint32_t The event flags (can be used to check which event occurred).
 *
 * @note This function modifies the global event flags in a critical section to
 *       ensure atomic access to shared data, preventing race conditions.
 */
Events_t getNextEvent(void) {

  Events_t theEvent;

  CORE_DECLARE_IRQ_STATE;

  CORE_ENTER_CRITICAL();

  if (event_flags != EVENT_NONE) {
      theEvent = event_flags; // Get the current event flags
      event_flags = EVENT_NONE;  // Clear the event flags
  }

  CORE_EXIT_CRITICAL();

  return (theEvent);
}

#if (DEVICE_IS_BLE_SERVER == 0)

void discovery_state_machine(sl_bt_msg_t *evt){

  uint32_t ext_sig = 0;
  sl_status_t rc;

  ext_sig = SL_BT_MSG_ID(evt->header);

  State_t currentState;
  static State_t nextState = STATE_discoverService;

  //Get ble data struct
  ble_data_struct_t* ble_params = get_ble_data_struct();

  currentState = nextState;

  switch(currentState){

    case STATE_discoverService:
      nextState = STATE_discoverService;
      if(ext_sig == sl_bt_evt_connection_opened_id){
          // Discover HTM Service on client
          rc = sl_bt_gatt_discover_primary_services_by_uuid(ble_params->connectionHandle,
                                                            sizeof(healthThermo_ID),
                                                            (const uint8_t*)healthThermo_ID);
          if(rc != SL_STATUS_OK){
              LOG_ERROR("Bluetooth: Service discovery error = %d\r\n", (unsigned int) rc);
          }
          nextState = STATE_discoverChar;
      }
      break;

    case STATE_discoverChar:
      nextState = STATE_discoverChar;
      // If current GATT command completed, discover HTM characteristic on client
      if(ext_sig == sl_bt_evt_gatt_procedure_completed_id){

          rc = sl_bt_gatt_discover_characteristics_by_uuid(evt->data.evt_gatt_procedure_completed.connection,
                                                           ble_params->serviceHandle,
                                                           sizeof(tempMeasure_ID),
                                                           (const uint8_t*)tempMeasure_ID);
          if(rc != SL_STATUS_OK){
              LOG_ERROR("Bluetooth: Characteristic discovery error = %d\r\n", (unsigned int) rc);
          }
          nextState = STATE_setIndication;
      }

      else if(ext_sig == sl_bt_evt_connection_closed_id){
          nextState = STATE_discoverService;
      }
      break;

    case STATE_setIndication:
      nextState = STATE_setIndication;
      // If current GATT command completed, enable indications on client
      if(ext_sig == sl_bt_evt_gatt_procedure_completed_id){

          rc = sl_bt_gatt_set_characteristic_notification(evt->data.evt_gatt_procedure_completed.connection,
                                                          ble_params->characteristicHandle,
                                                          sl_bt_gatt_indication );
          if(rc != SL_STATUS_OK){
              LOG_ERROR("Bluetooth: Set characteristic indication error = %d\r\n", (unsigned int) rc);
          }
          nextState = STATE_getIndication;
      }
      else if(ext_sig == sl_bt_evt_connection_closed_id){
          nextState = STATE_discoverService;
      }
      break;

    case STATE_getIndication:
      nextState = STATE_getIndication;
      //If current GATT command completed
      if(ext_sig == sl_bt_evt_gatt_procedure_completed_id){

          nextState = STATE_startScanning;
     }
     else if(ext_sig == sl_bt_evt_connection_closed_id){
         nextState = STATE_discoverService;
     }
     break;

    case STATE_startScanning:
      nextState = STATE_startScanning;
      //If connection closed any time
      if(ext_sig == sl_bt_evt_connection_closed_id){
          nextState = STATE_discoverService;
      }
      break;

  }// switch

}

#else

bool stateMachinePostureDetection(sl_bt_msg_t *levt){

  uint32_t ext_sig = 0;
  static StatesP_t next_state = IDLE;

  if(SL_BT_MSG_ID(levt->header) ==  sl_bt_evt_system_external_signal_id){
      ext_sig =  levt->data.evt_system_external_signal.extsignals;
  }else{
      return false;
  }

  ble_data_struct_t* ble_params = get_ble_data_struct();

  //Stop taking temperature measurement if BLE connection is closed or HTM indications are disabled.
  if(ble_params->connection_open == false){
      next_state = IDLE;
      return true;
  }

  switch (next_state){
    case IDLE:
      //sl_power_manager_add_em_requirement(SL_POWER_MANAGER_EM3);
      next_state = WAIT_ACCELINT;
      break;

    case WAIT_ACCELINT:
      if(ext_sig == EVENT_ACCELINT){
          //LOG_INFO(" Trasitioning from STATE_IDLE to WAIT_ACCELINT\n\r");
          displayPrintf(DISPLAY_ROW_11, "Tilt:true");
          // Start first conversion
          ADC_Start(ADC0, adcStartScan);
          next_state = WAIT_ACCELINT;
      }
      break;

    case STATE_ADCON:
      if(ext_sig == EVENT_BLEDONE){
          //LOG_INFO(" Trasitioning from WAIT_ACCELINT to STATE_ADCON\n\r");
          displayPrintf(DISPLAY_ROW_11, "Tilt:false");
          //sl_power_manager_add_em_requirement(SL_POWER_MANAGER_EM3);
          next_state = WAIT_ACCELINT;
      }
      break;
  }

  return true;

}


#if 0
/**
 * @brief Handles the state machine for temperature measurement using the Si7021 sensor.
 *
 * This function processes BLE external signals and controls the sequence of operations
 * for reading temperature data from the Si7021 sensor over I2C. It transitions through
 * different states based on events such as timer underflow, I2C transfer completion,
 * and BLE connection status.
 *
 * @param levt Pointer to the BLE event message structure.
 * @return true if the state machine executed successfully, false if the event is invalid.
 */
bool stateMachineTemperatureRead(sl_bt_msg_t *levt){

  uint32_t ext_sig = 0;

  if(SL_BT_MSG_ID(levt->header) ==  sl_bt_evt_system_external_signal_id){
      ext_sig =  levt->data.evt_system_external_signal.extsignals;
  }else{
      return false;
  }

  ble_data_struct_t* ble_params = get_ble_data_struct();

  States_t current_state;
  static States_t next_state = STATE_IDLE;

  //Stop taking temperature measurement if BLE connection is closed or HTM indications are disabled.
  if(ble_params->connection_open == false || ble_params->ok_to_send_htm_connections == false){
      next_state = STATE_IDLE;
      // Clear Temperature print from LCD in this case
      displayPrintf(DISPLAY_ROW_TEMPVALUE, " ");
      return true;
  }

  current_state = next_state;

  switch (current_state) {

    case STATE_IDLE:
      next_state = STATE_IDLE;
      if(ext_sig == EVENT_LETIMER_UF){ // event underflow
          //LOG_INFO(" Trasitioning from STATE_IDLE to STATE_SENSOROn\n\r");
          next_state = STATE_SENSOROn;
          //Turn on power to the Si7021 and wait for 80ms
          powerOnSi7021();
      }

      if(ext_sig == EVENT_BLEConnectionClose){
          //LOG_ERROR("Error: EVENT_BLEConnectionClose\r\n");
          powerOffSi7021();
          next_state = STATE_IDLE;
      }
      break;

    case STATE_SENSOROn:
      next_state = STATE_SENSOROn;
      if(ext_sig == EVENT_LETIMER_COMP1){ // event COMP1
          //LOG_INFO(" Trasitioning from STATE_SENSOROn to STATE_I2CWrite\n\r");
          next_state = STATE_I2CWrite;
          //sl_power_manager_add_em_requirement(SL_POWER_MANAGER_EM1);
          //Send temperature read cmd to Si7021
          I2CTransferWrite();
      }

      if(ext_sig == EVENT_BLEConnectionClose){
          //LOG_ERROR("Error: EVENT_BLEConnectionClose\r\n");
          powerOffSi7021();
          next_state = STATE_IDLE;
      }
      break;

    case STATE_I2CWrite:
      next_state = STATE_I2CWrite;
      if(ext_sig == EVENT_I2CTransfer_Done){
         // LOG_INFO(" Trasitioning from STATE_I2CWrite to STATEs_I2CRead\n\r");
          //sl_power_manager_remove_em_requirement(SL_POWER_MANAGER_EM1);
          next_state = STATE_I2CRead;
          NVIC_DisableIRQ(I2C0_IRQn);
          //Wait for 10.8ms
          conversionWaitTimeSi7021();
      }

      if(ext_sig == EVENT_BLEConnectionClose){
          //LOG_ERROR("Error: EVENT_BLEConnectionClose\r\n");
          powerOffSi7021();
          next_state = STATE_IDLE;
      }
      break;

    case STATE_I2CRead:
      next_state = STATE_I2CRead;
      if(ext_sig == EVENT_LETIMER_COMP1){
         // LOG_INFO(" Trasitioning from STATEs_I2CRead to STATE_GETResult\n\r");
          next_state = STATE_GETResult;
//          sl_power_manager_add_em_requirement(SL_POWER_MANAGER_EM1);
          // Read two bytes of temperature from Si7021
          I2CTransferRead();
      }

      if(ext_sig == EVENT_BLEConnectionClose){
          //LOG_ERROR("Error: EVENT_BLEConnectionClose\r\n");
          powerOffSi7021();
          next_state = STATE_IDLE;
      }
      break;

    case STATE_GETResult:
      next_state = STATE_GETResult;
      if(ext_sig == EVENT_I2CTransfer_Done){
         // LOG_INFO(" Trasitioning from STATE_GETResult to STATE_IDLE\n\r");
//          sl_power_manager_remove_em_requirement(SL_POWER_MANAGER_EM1);
          next_state = STATE_IDLE;
          NVIC_DisableIRQ(I2C0_IRQn);
          // turn off Si7021 sensor
          //powerOffSi7021();
          //print the temperature value in degrees
          printTemperatureSi7021();
          // send temperature value over BLE
          send_temp_ble();
      }

      if(ext_sig == EVENT_BLEConnectionClose){
          //LOG_ERROR("Error: EVENT_BLEConnectionClose\r\n");
          powerOffSi7021();
          next_state = STATE_IDLE;
      }
      break;
  }

  return true;
}

#endif

#endif
