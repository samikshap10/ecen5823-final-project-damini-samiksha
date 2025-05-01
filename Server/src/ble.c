/***********************************************************************
 * @file      ble.c
 * @version   0.1
 * @brief     Function header/interface file.
 *
 * @author    Damini Gowda, damini.gowda@colorado.edu
 * @date      Feb 16, 2025
 *
 *
 * @institution University of Colorado Boulder (UCB)
 * @course      ECEN 5823-001: IoT Embedded Firmware
 * @instructor  Chris Choi
 *
 * @assignment Assignment 5 - BLE Health Thermometer Profile (HTP)
 * @due
 *
 * @resources EFR32xG13 Wireless GeckoReference Manual, Lecture PDFs, AN and reading references
 *
 */
#define INCLUDE_LOG_DEBUG     1

#include "ble.h"

#define ENABLE_BLE_LOGS                     0
#define ENABLE_ERROR_LOGS                   1
#define MIN_ADVERTISE_INTERVAL             250
#define MIN_ADVERTISE_INTERVAL_VAL         (MIN_ADVERTISE_INTERVAL)/0.625
#define MAX_ADVERTISE_INTERVAL             250
#define MAX_ADVERTISE_INTERVAL_VAL         (MAX_ADVERTISE_INTERVAL)/0.625
#define MIN_CONNECTION_INTERVAL            75 //(60 * 1.25 = 75ms)
#define MIN_CONNECTION_INTERVAL_VAL        (MIN_CONNECTION_INTERVAL/1.25)
#define MAX_CONNECTION_INTERVAL            75 //(60 * 1.25 = 75ms)
#define MAX_CONNECTION_INTERVAL_VAL        (MAX_CONNECTION_INTERVAL/1.25)
#define MAX_LATENCY                        300   //(4 * 75 = 300ms)
#define MAX_LATENCY_VAL                    (MAX_LATENCY/MAX_CONNECTION_INTERVAL)
#define MAX_TIMEOUT_VAL                    ((1 + MAX_LATENCY_VAL) * (MAX_CONNECTION_INTERVAL * 2) + MAX_CONNECTION_INTERVAL)
#define MIN_CE_LENGTH                       0
#define MAX_CE_LENGTH                       4

#define SCAN_INTERVAL                       50 //ms
#define SCAN_INTERVAL_VAL                   (SCAN_INTERVAL/0.625)
#define SCAN_WINDOW                         25 //ms

#define BUTTON_PRESSED 0x01
#define BUTTON_RELEASED 0x00

#define MAX_QUEUE_SIZE 16

uint8_t flexData=0;
uint8_t accelData=0;
uint8_t flag=0;

typedef struct {
  uint16_t charHandle;      // Characteristic handle from gatt_db.h
  size_t bufferLength;      // Length of the buffer
  uint8_t buffer[5];        // Data buffer (max size 5 bytes for HTM + button state)
} indication_t;

typedef struct {
  indication_t queue[MAX_QUEUE_SIZE];  // Circular queue for storing indications
  uint8_t readPointer;                 // Index to the oldest entry
  uint8_t writePointer;                // Index to the newest entry
  uint8_t size;                        // Number of indications in the queue
} indication_queue_t;

indication_queue_t indicationQueue;

bool enqueue(indication_queue_t* q, indication_t indication) {
  if (q->size < MAX_QUEUE_SIZE) {
      q->queue[q->writePointer] = indication;
      q->writePointer = (q->writePointer + 1) % MAX_QUEUE_SIZE;
      q->size++;
      return true;
  } else {
      // Queue is full, discard or overwrite the oldest indication
      q->readPointer = (q->readPointer + 1) % MAX_QUEUE_SIZE;
      q->queue[q->writePointer] = indication;
      q->writePointer = (q->writePointer + 1) % MAX_QUEUE_SIZE;
      return false;  // Indication discarded
  }
}

bool dequeue(indication_queue_t* q, indication_t* indication) {
  if (q->size == 0) {
      return false;  // Queue is empty
  }
  *indication = q->queue[q->readPointer];
  q->readPointer = (q->readPointer + 1) % MAX_QUEUE_SIZE;
  q->size--;
  return true;
}

static ble_data_struct_t ble_data = {.advertisingSetHandle = 0xff};

int32_t FLOAT_TO_INT32(const uint8_t *value_start_little_endian);

ble_data_struct_t*  get_ble_data_struct(void){

  return &ble_data;
}

/**
 * @brief Handles BLE events and manages the Bluetooth operations based on the event type.
 *
 * This function processes different Bluetooth events, such as boot, connection, disconnection,
 * changes in connection parameters, and GATT server status updates. It performs actions like
 * advertising, managing connections, and handling characteristics' configurations.
 *
 * @param evt Pointer to the BLE event received from the Bluetooth stack.
 */
void handle_ble_event(sl_bt_msg_t *evt){

  sl_status_t rc;

#if (DEVICE_IS_BLE_SERVER == 0)
  uint8_t serverAddress[] = SERVER_BT_ADDRESS;
  uint8_t* charValue;
#endif

  switch (SL_BT_MSG_ID(evt->header)) {

#if DEVICE_IS_BLE_SERVER == 1
    /* Event received when the device has started and the radio is ready.*/
    case sl_bt_evt_system_boot_id:

#if ENABLE_BLE_LOGS
      // Print boot message.
      LOG_INFO("Bluetooth stack booted: v%d.%d.%d-b%d\n\r",
               evt->data.evt_system_boot.major,
               evt->data.evt_system_boot.minor,
               evt->data.evt_system_boot.patch,
               evt->data.evt_system_boot.build);
#endif

      // Initialize connection state
      ble_data.connection_open = false;
      ble_data.ok_to_send_htm_connections = false;
      ble_data.ok_to_send_bs_connections = false;
      ble_data.indication_in_flight =false;
      ble_data.expecting_passkey_confirmation = false;

      // 1. Read the Bluetooth identity address used by the device
      rc = sl_bt_system_get_identity_address(&ble_data.myAddress, NULL);
      if(rc != SL_STATUS_OK){
#if ENABLE_ERROR_LOGS
          LOG_ERROR("Bluetooth: Get Identity error = %d\r\n", (unsigned int) rc);
#endif
      }

      // 2. Create bluetooth advertiser set
      rc = sl_bt_advertiser_create_set(&ble_data.advertisingSetHandle);
      if(rc != SL_STATUS_OK){
#if ENABLE_ERROR_LOGS
          LOG_ERROR("Bluetooth: Create advertiser set error = %d\r\n", (unsigned int) rc);
#endif
      }

      // 3. Set advertising interval to 100ms
      rc = sl_bt_advertiser_set_timing(
          ble_data.advertisingSetHandle, // advertising set handle
          MIN_ADVERTISE_INTERVAL_VAL, // min. adv. interval (milliseconds * 1.6)
          MAX_ADVERTISE_INTERVAL_VAL, // max. adv. interval (milliseconds * 1.6)
          0,   // adv. duration
          0);  // max. num. adv. events
      if(rc != SL_STATUS_OK){
#if ENABLE_ERROR_LOGS
          LOG_ERROR("Bluetooth: Set advertiser timing error = %d\r\n", (unsigned int) rc);
#endif
      }

      rc = sl_bt_legacy_advertiser_generate_data(
          ble_data.advertisingSetHandle,
          sl_bt_advertiser_general_discoverable);
      if(rc != SL_STATUS_OK){
#if ENABLE_ERROR_LOGS
          LOG_ERROR("Bluetooth: advertiser generate data = %d\r\n", (unsigned int) rc);
#endif
      }

      // 4. Start advertising on the advertising set with the specified discovery and connection modes.
      rc = sl_bt_legacy_advertiser_start(
          ble_data.advertisingSetHandle,
          sl_bt_legacy_advertiser_connectable);

      if(rc != SL_STATUS_OK){
#if ENABLE_ERROR_LOGS
          LOG_ERROR("Bluetooth: Advertiser start error = %d\r\n", (unsigned int) rc);
#endif
      }

      // Initialize the display
      displayInit();

      // Add LCD prints
      displayPrintf(DISPLAY_ROW_NAME, BLE_DEVICE_TYPE_STRING);
      displayPrintf(DISPLAY_ROW_BTADDR, "%X:%X:%X:%X:%X:%X",ble_data.myAddress.addr[0], ble_data.myAddress.addr[1],
                    ble_data.myAddress.addr[2], ble_data.myAddress.addr[3], ble_data.myAddress.addr[4], ble_data.myAddress.addr[5]);
      displayPrintf(DISPLAY_ROW_CONNECTION, ADVERTISING_STRING);
      displayPrintf(DISPLAY_ROW_ASSIGNMENT, ASSIGNMENT_NUMBER);
      displayPrintf(DISPLAY_ROW_9, "FlexAngle: 0 Deg");
      displayPrintf(DISPLAY_ROW_10, "Tilt Count:%d",accelData);

#if ENABLE_BLE_LOGS
      LOG_INFO("Advertising started...\r\n");
#endif

      rc = sl_bt_sm_delete_bondings();

      if(rc != SL_STATUS_OK){
#if ENABLE_ERROR_LOGS
          LOG_ERROR("Bluetooth: sl_bt_sm_delete_bondings error = %d\r\n", (unsigned int) rc);
#endif
      }

      rc = sl_bt_sm_configure(0x0F, sm_io_capability_displayyesno);

      if(rc != SL_STATUS_OK){
#if ENABLE_ERROR_LOGS
          LOG_ERROR("Bluetooth: sl_bt_sm_delete_bondings error = %d\r\n", (unsigned int) rc);
#endif
      }

      break;

      /*Indication of a new connection opening.*/
    case sl_bt_evt_connection_opened_id:

#if ENABLE_BLE_LOGS
      LOG_INFO("New connection opened...\r\n");
#endif

      // Save connection Handle
      ble_data.connectionHandle = evt->data.evt_connection_opened.connection;

      // 1. Stop the advertisement
      rc = sl_bt_advertiser_stop(ble_data.advertisingSetHandle);
      if(rc != SL_STATUS_OK){
#if ENABLE_ERROR_LOGS
          LOG_ERROR("Bluetooth: Advertiser stop error = %d\r\n", (unsigned int) rc);
#endif
      }

      // Update the connection state
      ble_data.connection_open = true;
      ble_data.ok_to_send_htm_connections = false;
      ble_data.indication_in_flight =false;

#if ENABLE_BLE_LOGS
      LOG_INFO("connection_open is true...\r\n");
#endif
      // 2. Set connection parameters
      rc = sl_bt_connection_set_parameters(ble_data.connectionHandle,
                                           MIN_CONNECTION_INTERVAL_VAL,
                                           MAX_CONNECTION_INTERVAL_VAL,
                                           MAX_LATENCY_VAL,
                                           80,
                                           0,
                                           0xffff);
      if(rc != SL_STATUS_OK){
#if ENABLE_ERROR_LOGS
          LOG_ERROR("Bluetooth: Connection set parameter = 0x%X\r\n", (unsigned int) rc);
#endif
      }

      // Add LCD prints
      displayPrintf(DISPLAY_ROW_CONNECTION, CONNECTED_STRING);

      break;

      /*This event indicates that a connection was closed*/
    case sl_bt_evt_connection_closed_id:

#if ENABLE_BLE_LOGS
      LOG_INFO("Connection closed!\r\n");
#endif
      // 1. Set the connection lost event
      schedulerSetEventBleConnectionClose();

      // 2. Set current connection open status & ok_to_send_indication to false
      ble_data.connection_open = false;
      ble_data.ok_to_send_htm_connections = false;
      ble_data.indication_in_flight = false;

#if ENABLE_BLE_LOGS
      LOG_INFO("connection_open is false...\r\n");
#endif

      rc = sl_bt_legacy_advertiser_generate_data(
          ble_data.advertisingSetHandle,
          sl_bt_advertiser_general_discoverable);
      if(rc != SL_STATUS_OK){
#if ENABLE_ERROR_LOGS
          LOG_ERROR("Bluetooth: advertiser generate data = %d\r\n", (unsigned int) rc);
#endif
      }

      // 4. Start advertising on the advertising set with the specified discovery and connection modes.
      rc = sl_bt_legacy_advertiser_start(
          ble_data.advertisingSetHandle,
          sl_bt_legacy_advertiser_connectable);

      if(rc != SL_STATUS_OK){
          LOG_ERROR("Bluetooth: Advertiser start error = %d\r\n", (unsigned int) rc);
      }

      rc = sl_bt_sm_delete_bondings();

      if(rc != SL_STATUS_OK){
#if ENABLE_ERROR_LOGS
          LOG_ERROR("Bluetooth: sl_bt_sm_delete_bondings error = %d\r\n", (unsigned int) rc);
#endif
      }

      // Add LCD prints.
      displayPrintf(DISPLAY_ROW_CONNECTION, ADVERTISING_STRING);
      displayPrintf(DISPLAY_ROW_TEMPVALUE, " ");


#if ENABLE_BLE_LOGS
      LOG_INFO("Advertising started...\r\n");
#endif

      break;

      /*Informational. Triggered whenever the connection parameters are changed and at any time a connection is established*/
    case sl_bt_evt_connection_parameters_id:

#if ENABLE_BLE_LOGS
      LOG_INFO("Connection parameters changed/connection established\r\n");
      LOG_INFO("Ble connection parameters:\n\r");
      LOG_INFO("HANDLE - %d\r\n", evt->data.evt_connection_parameters.connection);
      int interval = (int) (evt->data.evt_connection_parameters.interval*1.25);
      LOG_INFO("INTERVAL - %d\r\n", interval);
      LOG_INFO("LATENCY - %d\r\n", evt->data.evt_connection_parameters.latency);
      LOG_INFO("TIMEOUT - %d\r\n", evt->data.evt_connection_parameters.timeout*10);
#endif

      break;

      /*Indicates either:
      A local Client Characteristic Configuration descriptor (CCCD) was changed by the remote GATT client, or
      That a confirmation from the remote GATT Client was received upon a successful reception of the indication
      I.e. we sent an indication from our server to the client with sl_bt_gatt_server_send_indication()*/
    case sl_bt_evt_gatt_server_characteristic_status_id:

      //For button  state characteristic, see if client characteristic configuration changed
      if((evt->data.evt_gatt_server_characteristic_status.characteristic == gattdb_flex_data)
          && (evt->data.evt_gatt_server_characteristic_status.status_flags == sl_bt_gatt_server_client_config)){

          //Check if indications were enabled from the client
          if(evt->data.evt_gatt_server_characteristic_status.client_config_flags == 0x02){
              ble_data.ok_to_send_bs_connections = true;
#if ENABLE_BLE_LOGS
              LOG_INFO("ok_to_send_htm_connections is true...\r\n");
#endif
          }

          //Check if indications were disabled from the client
          if(evt->data.evt_gatt_server_characteristic_status.client_config_flags == 0x00){
              ble_data.ok_to_send_bs_connections = false;
#if ENABLE_BLE_LOGS
              LOG_INFO("ok_to_send_htm_connections is false...\r\n");
#endif
          }

      }

      //For button  state characteristic, see if client characteristic configuration changed
      if((evt->data.evt_gatt_server_characteristic_status.characteristic == gattdb_accelerometer_data)
          && (evt->data.evt_gatt_server_characteristic_status.status_flags == sl_bt_gatt_server_client_config)){

          //Check if indications were enabled from the client
          if(evt->data.evt_gatt_server_characteristic_status.client_config_flags == 0x02){
              ble_data.ok_to_send_bs_connections = true;
#if ENABLE_BLE_LOGS
              LOG_INFO("ok_to_send_htm_connections is true...\r\n");
#endif
          }

          //Check if indications were disabled from the client
          if(evt->data.evt_gatt_server_characteristic_status.client_config_flags == 0x00){
              ble_data.ok_to_send_bs_connections = false;
#if ENABLE_BLE_LOGS
              LOG_INFO("ok_to_send_htm_connections is false...\r\n");
#endif
          }

      }

      if((evt->data.evt_gatt_server_characteristic_status.characteristic == gattdb_flex_data)
          && (evt->data.evt_gatt_server_characteristic_status.status_flags == sl_bt_gatt_server_confirmation)){
          ble_data.indication_in_flight = false;

          indication_t dequeuedIndication;

          if(dequeue(&indicationQueue, &dequeuedIndication)){
              if(dequeuedIndication.charHandle == gattdb_flex_data){
                  // Send the indication to the client
                  rc = sl_bt_gatt_server_send_indication(
                      ble_data.connectionHandle,
                      gattdb_flex_data, // handle from gatt_db.h
                      1,
                      &dequeuedIndication.buffer[0] // in IEEE-11073 format
                  );
                  if (rc != SL_STATUS_OK) {
#if ENABLE_ERROR_LOGS
                      LOG_ERROR("Error Sending client data:%X\r\n",(unsigned int) rc);
#endif
                  } else {
                      // Set the flag that an indication is in flight
                      ble_data.indication_in_flight = true;
                  }
              }
          }
#if ENABLE_BLE_LOGS
          LOG_INFO("indication_in_flight is false...\r\n");
#endif
      }

      if((evt->data.evt_gatt_server_characteristic_status.characteristic == gattdb_accelerometer_data)
          && (evt->data.evt_gatt_server_characteristic_status.status_flags == sl_bt_gatt_server_confirmation)){
          ble_data.indication_in_flight = false;

          indication_t dequeuedIndication;

          if(dequeue(&indicationQueue, &dequeuedIndication)){
              if(dequeuedIndication.charHandle == gattdb_accelerometer_data){
                  // Send the indication to the client
                  rc = sl_bt_gatt_server_send_indication(
                      ble_data.connectionHandle,
                      gattdb_accelerometer_data, // handle from gatt_db.h
                      1,
                      &dequeuedIndication.buffer[0] // in IEEE-11073 format
                  );
                  if (rc != SL_STATUS_OK) {
#if ENABLE_ERROR_LOGS
                      LOG_ERROR("Error Sending client data:%X\r\n",(unsigned int) rc);
#endif
                  } else {
                      // Set the flag that an indication is in flight
                      ble_data.indication_in_flight = true;
                  }
              }
          }
#if ENABLE_BLE_LOGS
          LOG_INFO("indication_in_flight is false...\r\n");
#endif
      }

      break;

    case sl_bt_evt_system_external_signal_id:

      if (evt->data.evt_system_external_signal.extsignals == EVENT_PB0) {
#if ENABLE_BLE_LOGS
          LOG_INFO("Buton event...\r\n");
#endif
          if(ble_data.expecting_passkey_confirmation == true){
              // Confirm pairing when PB0 is pressed
              sl_bt_sm_passkey_confirm(ble_data.connectionHandle, 1);
              ble_data.expecting_passkey_confirmation = false;
          }
      }
      if (evt->data.evt_system_external_signal.extsignals == EVENT_0DEGREE){
          flexData = 0;
          rc = sl_bt_gatt_server_write_attribute_value(gattdb_flex_data, 0, sizeof(uint8_t),&flexData);
          if (rc == SL_STATUS_OK && ble_data.bonded && ble_data.connection_open) {
              send_next_indication_flex(flexData);
          }
          displayPrintf(DISPLAY_ROW_9, "Flex Angle:0Deg");
          accelData++;
          rc = sl_bt_gatt_server_write_attribute_value(gattdb_accelerometer_data, 0, sizeof(uint8_t),&accelData);
          if (rc == SL_STATUS_OK && ble_data.bonded && ble_data.connection_open) {
              send_next_indication_accel(accelData);
          }
          displayPrintf(DISPLAY_ROW_10, "Tilt Count:%d",accelData);
      }
      else if (evt->data.evt_system_external_signal.extsignals == EVENT_45DEGREE){
          flexData = 45;
          rc = sl_bt_gatt_server_write_attribute_value(gattdb_flex_data, 0, sizeof(uint8_t), &flexData);
          if (rc == SL_STATUS_OK && ble_data.bonded && ble_data.connection_open) {
              send_next_indication_flex(flexData);
          }
          displayPrintf(DISPLAY_ROW_9, "Flex Angle:45Deg");
          accelData++;
          rc = sl_bt_gatt_server_write_attribute_value(gattdb_accelerometer_data, 0, sizeof(uint8_t),&accelData);
          if (rc == SL_STATUS_OK && ble_data.bonded && ble_data.connection_open) {
              send_next_indication_accel(accelData);
          }
          displayPrintf(DISPLAY_ROW_10, "Tilt Count:%d",accelData);
          schedulerSetEventBLEDONE();
      }
      else if (evt->data.evt_system_external_signal.extsignals == EVENT_90DEGREE){
          flexData = 90;
          rc = sl_bt_gatt_server_write_attribute_value(gattdb_flex_data, 0, sizeof(uint8_t), &flexData);
          if (rc == SL_STATUS_OK && ble_data.bonded && ble_data.connection_open) {
              send_next_indication_flex(flexData);
          }
          displayPrintf(DISPLAY_ROW_9, "Flex Angle:90Deg");
          accelData++;
          rc = sl_bt_gatt_server_write_attribute_value(gattdb_accelerometer_data, 0, sizeof(uint8_t),&accelData);
          if (rc == SL_STATUS_OK && ble_data.bonded && ble_data.connection_open) {
              send_next_indication_accel(accelData);
          }
          displayPrintf(DISPLAY_ROW_10, "Tilt Count:%d",accelData);
          schedulerSetEventBLEDONE();
      }
      break;

      /*Possible event from calling sl_bt_gatt_server_send_indication() - i.e. we never received a confirmation
       *  for a previously transmitted indication.*/
    case sl_bt_evt_gatt_server_indication_timeout_id:

#if ENABLE_BLE_LOGS
      LOG_INFO("Confirmation not received for a previously transmitted event\r\n");
#endif

      //Check if an indication is in flight, If yes then reset it since a timeout has occurred
      if(ble_data.indication_in_flight == true){
          ble_data.indication_in_flight = false;
#if ENABLE_BLE_LOGS
          LOG_INFO("indication_in_flight is false...\r\n");
#endif
      }

      break;

      /*Indicates that a soft timer has lapsed.*/
    case sl_bt_evt_system_soft_timer_id:

      if(evt->data.evt_system_soft_timer.handle == TIMER_HANDLE_1HZ){
          // Update the display @ 1Hz
          displayUpdate();
      }


      break;

    case sl_bt_evt_sm_confirm_passkey_id:
#if ENABLE_BLE_LOGS
      LOG_INFO("Expecting Passkey confirmation\r\n");
#endif
      displayPrintf(DISPLAY_ROW_PASSKEY, "%06lu", evt->data.evt_sm_confirm_passkey.passkey);
      displayPrintf(DISPLAY_ROW_ACTION, "Confirm with PB0");
      ble_data.expecting_passkey_confirmation = true;
      break;

    case sl_bt_evt_sm_confirm_bonding_id:
#if ENABLE_BLE_LOGS
      LOG_INFO("Bonding Confirmation...\r\n");
#endif
      sl_bt_sm_bonding_confirm(evt->data.evt_sm_confirm_bonding.connection, 1);
      break;

    case sl_bt_evt_sm_bonded_id:
#if ENABLE_BLE_LOGS
      LOG_INFO("Bonded...\r\n");
#endif
      ble_data.bonded = true;
      displayPrintf(DISPLAY_ROW_ACTION, " ");
      displayPrintf(DISPLAY_ROW_PASSKEY, " ");
      displayPrintf(DISPLAY_ROW_CONNECTION, "Bonded");
      break;

    case sl_bt_evt_sm_bonding_failed_id:
#if ENABLE_BLE_LOGS
      LOG_INFO("Bonding Failed...\r\n");
#endif
      ble_data.bonded = false;
      displayPrintf(DISPLAY_ROW_CONNECTION, "Bonding Failed");
      break;
#else
      /*This event is received when the device has started and
      the radio is ready.*/
    case sl_bt_evt_system_boot_id:

#if ENABLE_BLE_LOGS
      // Print boot message.
      LOG_INFO("Bluetooth stack booted: v%d.%d.%d-b%d\n\r",
               evt->data.evt_system_boot.major,
               evt->data.evt_system_boot.minor,
               evt->data.evt_system_boot.patch,
               evt->data.evt_system_boot.build);
#endif

      // Set scan parameters for subsequent scanning operations
      rc = sl_bt_scanner_set_parameters(sl_bt_scanner_scan_mode_passive,SCAN_INTERVAL_VAL,SCAN_WINDOW);

      if(rc != SL_STATUS_OK){
          LOG_ERROR("Bluetooth: Scanner Set parameters = %d\r\n", (unsigned int) rc);
      }

      // Sets default connection parameters for all subsequent connections
      rc = sl_bt_connection_set_default_parameters(
          MIN_CONNECTION_INTERVAL_VAL,
          MAX_CONNECTION_INTERVAL_VAL,
          4,
          MAX_TIMEOUT_VAL,
          MIN_CE_LENGTH,
          MAX_CE_LENGTH);

      if(rc != SL_STATUS_OK){
          LOG_ERROR("Bluetooth: Set connection default parameters error = %d\r\n", (unsigned int) rc);
      }

      // Start GAP discovery procedure to scan for advertising devices
      rc = sl_bt_scanner_start(sl_bt_gap_1m_phy , sl_bt_scanner_discover_generic);
      if(rc != SL_STATUS_OK){
          LOG_ERROR("Bluetooth: Scanner start error = %d\r\n", (unsigned int) rc);
      }

#if ENABLE_BLE_LOGS
      LOG_INFO("Scanning started.\r\n");
#endif

      // Initialize the display
      displayInit();

      // Read the bluetooth identity address used by the device
      rc = sl_bt_system_get_identity_address(&ble_data.myAddress, sl_bt_gap_public_address);
      if(rc != SL_STATUS_OK){
          LOG_ERROR("Bluetooth: Get Identity error = %d\r\n", (unsigned int) rc);
      }

      // Add LCD prints
      displayPrintf(DISPLAY_ROW_NAME, BLE_DEVICE_TYPE_STRING);
      displayPrintf(DISPLAY_ROW_BTADDR, "%X:%X:%X:%X:%X:%X",ble_data.myAddress.addr[0], ble_data.myAddress.addr[1],
                    ble_data.myAddress.addr[2], ble_data.myAddress.addr[3], ble_data.myAddress.addr[4], ble_data.myAddress.addr[5]);
      displayPrintf(DISPLAY_ROW_ASSIGNMENT, ASSIGNMENT_NUMBER);
      displayPrintf(DISPLAY_ROW_CONNECTION, DISCOVERING_STRING);

      ble_data.myAddress.addr[0]= serverAddress[0];
      ble_data.myAddress.addr[1]= serverAddress[1];
      ble_data.myAddress.addr[2]= serverAddress[2];
      ble_data.myAddress.addr[3]= serverAddress[3];
      ble_data.myAddress.addr[4]= serverAddress[4];
      ble_data.myAddress.addr[5]= serverAddress[5];

      // Open connection with the server
      rc = sl_bt_connection_open(ble_data.myAddress,
                                 ble_data.myAddressType,
                                 sl_bt_gap_phy_1m,
                                 (uint8_t *)1);
      if(rc != SL_STATUS_OK){
          LOG_ERROR("Bluetooth: Connection open error = %d\r\n", (unsigned int) rc);
      }

      break;

      /* This event is generated when a new connection is established.*/
    case sl_bt_evt_connection_opened_id:

#if ENABLE_BLE_LOGS
      LOG_INFO("New connection opened...\r\n");
#endif

      // Save connection Handle
      ble_data.connectionHandle = evt->data.evt_connection_opened.connection;

      // Add LCD prints
      displayPrintf(DISPLAY_ROW_CONNECTION, CONNECTED_STRING);
      displayPrintf(DISPLAY_ROW_BTADDR2, "%X:%X:%X:%X:%X:%X",serverAddress[0], serverAddress[1],
                    serverAddress[2], serverAddress[3], serverAddress[4], serverAddress[5]);

      break;

      /*This event indicates that a connection was closed.*/
    case sl_bt_evt_connection_closed_id:

#if ENABLE_BLE_LOGS
      LOG_INFO("Connection closed!\r\n");
#endif

      // Restart GAP discovery procedure to scan for advertising devices
      rc = sl_bt_scanner_start(sl_bt_gap_1m_phy , sl_bt_scanner_discover_generic);
      if(rc != SL_STATUS_OK){
          LOG_ERROR("Bluetooth: Scanner start error = %d\r\n", (unsigned int) rc);
      }

      // Open connection with the server
      rc = sl_bt_connection_open(ble_data.myAddress,
                                 ble_data.myAddressType,
                                 sl_bt_gap_phy_1m,
                                 (uint8_t *)1);
      if(rc != SL_STATUS_OK){
          LOG_ERROR("Bluetooth: Connection open error = %d\r\n", (unsigned int) rc);
      }

      // Add LCD prints
      displayPrintf(DISPLAY_ROW_CONNECTION, DISCOVERING_STRING);
      displayPrintf(DISPLAY_ROW_BTADDR2, " ");
      displayPrintf(DISPLAY_ROW_TEMPVALUE, " ");

      break;

      /* Received for advertising or scan response packets generated by: sl_bt_scanner_start().*/
    case sl_bt_evt_scanner_legacy_advertisement_report_id://sl_bt_evt_scanner_scan_report_id:

#if ENABLE_BLE_LOGS
      LOG_INFO("Scan response received.\r\n");
#endif

      // Check for bd_addr, packet_type and address_type from the server
      ble_data.myAddress = evt->data.evt_scanner_scan_report.address;
      ble_data.myAddressType = evt->data.evt_scanner_scan_report.address_type;
      ble_data.packetType = evt->data.evt_scanner_legacy_advertisement_report.event_flags;

#if ENABLE_BLE_LOGS
      LOG_INFO("Scan report address_type %u advertisement report event_flags %u\r\n",ble_data.myAddressType,ble_data.packetType);
#endif

      if(memcmp((void *) serverAddress, (void *) ble_data.myAddress.addr , sizeof(ble_data.myAddress)) == 0){
          // if(ble_data.myAddressType == 1){
          //     if(ble_data.packetType == 3){
          // Stop Scanning
          rc = sl_bt_scanner_stop();
          if(rc != SL_STATUS_OK){
              LOG_ERROR("Bluetooth: Scanner Stop error = %d\r\n", (unsigned int) rc);
          }

          // Open connection with the server
          rc = sl_bt_connection_open(ble_data.myAddress,
                                     ble_data.myAddressType,
                                     sl_bt_gap_phy_1m,
                                     (uint8_t *)1);
          if(rc != SL_STATUS_OK){
              LOG_ERROR("Bluetooth: Connection open error = %d\r\n", (unsigned int) rc);
          }else{
#if ENABLE_BLE_LOGS
              LOG_INFO("Connection with server successful!\r\n");
#endif
          }
          //   }
          // }
      }

      break;

      /*We get this event when it’s ok to call the next GATT command.*/
    case sl_bt_evt_gatt_procedure_completed_id:

#if ENABLE_BLE_LOGS
      LOG_INFO("GATT Command completed.\r\n");
#endif

      break;

      /*This event is received when a GATT service in the remote GATT database was discovered. */
    case sl_bt_evt_gatt_service_id:

#if ENABLE_BLE_LOGS
      LOG_INFO("GATT service in remote GATT database discovered.\r\n");
#endif

      // Save the GATT HTM service handle
      ble_data.serviceHandle = evt->data.evt_gatt_service.service;

      break;

      /*This event is received when a GATT characteristic in the remote GATT database was discovered. */
    case sl_bt_evt_gatt_characteristic_id:

#if ENABLE_BLE_LOGS
      LOG_INFO("GATT characteristic in remote GATT database discovered.\r\n");
#endif

      // Save the GATT HTM characteristic handle
      ble_data.characteristicHandle = evt->data.evt_gatt_characteristic.characteristic;

      break;

      /*If an indication or notification has been enabled for a characteristic, this event is triggered
       * whenever an indication or notification is received from the remote GATT server */
    case sl_bt_evt_gatt_characteristic_value_id:

#if ENABLE_BLE_LOGS
      LOG_INFO("Characteristic value received from remote.\r\n");
#endif

      // Is the characteristic handle and att_opcode we expect?
      if(evt->data.evt_gatt_characteristic_value.characteristic == gattdb_temperature_measurement &&
          evt->data.evt_gatt_characteristic_value.att_opcode == gatt_handle_value_indication ){

          // Send indication confirmation
          rc = sl_bt_gatt_send_characteristic_confirmation(ble_data.connectionHandle);
          if(rc != SL_STATUS_OK){
              LOG_ERROR("Bluetooth: Sending characteristic confirmation error = %d\r\n", (unsigned int) rc);
          }

          // Convert the temperature to be displayed on Client
          charValue = &(evt->data.evt_gatt_characteristic_value.value.data[0]);
          int32_t tempVal = FLOAT_TO_INT32(charValue);

          // Add LCD prints
          displayPrintf(DISPLAY_ROW_CONNECTION, HANDLING_STRING);
          displayPrintf(DISPLAY_ROW_TEMPVALUE, "Temperature=%d", tempVal);

      }

      break;

      /*Indicates that a soft timer has lapsed.*/
    case sl_bt_evt_system_soft_timer_id:

      //Update the display @ 1Hz
      displayUpdate();

      break;

#endif

  }// switch

}

/**
 * @brief Sends the next indication from the queue if available.
 *
 * This function checks if there is any pending indication in the queue and
 * sends it using the GATT server API. If the indication is sent successfully,
 * the queue is updated and the `indication_in_flight` flag is set.
 */
void send_next_indication_flex(uint8_t state) {

  indication_t newIndication;
  newIndication.charHandle = gattdb_flex_data;
  newIndication.bufferLength = 1;
  newIndication.buffer[0] = state;

  if (ble_data.indication_in_flight == false) {
      sl_status_t rc = sl_bt_gatt_server_send_indication(
          ble_data.connectionHandle,
          gattdb_flex_data,
          sizeof(uint8_t),
          &state
      );
      if (rc == SL_STATUS_OK) {
          ble_data.indication_in_flight = true;

      }
  }
  else{
      enqueue(&indicationQueue,newIndication);
  }
}

/**
 * @brief Sends the next indication from the queue if available.
 *
 * This function checks if there is any pending indication in the queue and
 * sends it using the GATT server API. If the indication is sent successfully,
 * the queue is updated and the `indication_in_flight` flag is set.
 */
void send_next_indication_accel(uint8_t state) {

  indication_t newIndication;
  newIndication.charHandle = gattdb_accelerometer_data;
  newIndication.bufferLength = 1;
  newIndication.buffer[0] = state;

  if (ble_data.indication_in_flight == false) {
      sl_status_t rc = sl_bt_gatt_server_send_indication(
          ble_data.connectionHandle,
          gattdb_accelerometer_data,
          sizeof(uint8_t),
          &state
      );
      if (rc == SL_STATUS_OK) {
          ble_data.indication_in_flight = true;
      }
  }
  else{
      enqueue(&indicationQueue,newIndication);
  }
}

#if 0
/**
 * @brief Sends the current temperature to the BLE client.
 *
 * This function retrieves the current temperature, writes it to the local GATT database,
 * performs necessary conversions to IEEE-11073 format, and sends the temperature data
 * as an indication to the connected BLE client if conditions permit.
 *
 * @note This function assumes that `returnTemp()` provides the current temperature
 * in Celsius and that necessary BLE configuration is already in place.
 */
void send_temp_ble(void){

  indication_t newIndication;
  uint32_t temperature_in_c, *temp;
  sl_status_t rc;

  uint8_t htm_temperature_buffer[5];
  uint8_t *p = htm_temperature_buffer;
  uint32_t htm_temperature_flt;
  uint8_t flag_byte = 0;

  // Get current temperature
  temp = returnTemp();
  temperature_in_c = *temp;

  //LOG_INFO("Temperature in C = %u\n\r", (unsigned int)temperature_in_c);

  // Write temperature to GATT database
  rc = sl_bt_gatt_server_write_attribute_value(
      gattdb_temperature_measurement, // GATT DB handle
      0, // offset
      4, // length
      (uint8_t *)&temperature_in_c // Pointer to the temperature data
  );
  if (rc != SL_STATUS_OK) {
#if ENABLE_ERROR_LOGS
      LOG_ERROR("Error writing GATT DB:%X\r\n", (unsigned int)rc);
#endif
  }

  // Convert temperature to IEEE-11073 forma
  htm_temperature_flt = UINT32_TO_FLOAT(temperature_in_c*1000, -3);

  // Convert temperature to bitstream and populate the buffer
  UINT8_TO_BITSTREAM(p, flag_byte);
  UINT32_TO_BITSTREAM(p, htm_temperature_flt);

  newIndication.charHandle = gattdb_temperature_measurement;
  newIndication.bufferLength = 5;
  memcpy(newIndication.buffer,htm_temperature_buffer,5);

  // Send data to client if conditions are met
  if (ble_data.ok_to_send_htm_connections == true
      && ble_data.connection_open == true
  ){

      // Ensure no other indication is in flight before sending
      if(ble_data.indication_in_flight == false){

          // Send the indication to the client
          rc = sl_bt_gatt_server_send_indication(
              ble_data.connectionHandle,
              gattdb_temperature_measurement, // handle from gatt_db.h
              5,
              &htm_temperature_buffer[0] // in IEEE-11073 format
          );
          if (rc != SL_STATUS_OK) {
#if ENABLE_ERROR_LOGS
              LOG_ERROR("Error Sending client data:%X\r\n",(unsigned int) rc);
#endif
          } else {
              // Set the flag that an indication is in flight
              ble_data.indication_in_flight = true;
          }

      }
      else{
          enqueue(&indicationQueue,newIndication);
      }

  }

}
#endif
/**
 * @brief Converts a floating-point value in a specific byte format to a 32-bit integer.
 *
 * This function extracts a floating-point value stored in a little-endian byte array,
 * interprets its mantissa and exponent, and converts it into an integer representation.
 *
 * @param[in] value_start_little_endian Pointer to the byte array containing the floating-point value.
 *                                      - Byte [0]: Flags byte (not used in this function).
 *                                      - Bytes [1-3]: Mantissa (stored in 2's complement format).
 *                                      - Byte [4]: Exponent (stored in 2's complement format).
 *
 * @return The converted 32-bit integer value.
 */
int32_t FLOAT_TO_INT32(const uint8_t *value_start_little_endian)
{
  uint8_t signByte = 0;
  int32_t mantissa;

  // input data format is:
  // [0] = flags byte
  // [3][2][1] = mantissa (2's complement)
  // [4] = exponent (2's complement)
  // BT value_start_little_endian[0] has the flags byte

  int8_t exponent = (int8_t)value_start_little_endian[4];

  // sign extend the mantissa value if the mantissa is negative
  if (value_start_little_endian[3] & 0x80) { // msb of [3] is the sign of the mantissa
      signByte = 0xFF;
  }

  mantissa = (int32_t) (value_start_little_endian[1] << 0) |
      (value_start_little_endian[2] << 8) |
      (value_start_little_endian[3] << 16) |
      (signByte << 24) ;

  // value = 10^exponent * mantissa, pow() returns a double type
  return (int32_t) (pow(10, exponent) * mantissa);

}
