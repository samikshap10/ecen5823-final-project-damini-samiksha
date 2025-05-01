/*
  File: ble_handler.c

  Author: <Your Name>
  Description:
   This file (`ble_handler.c`) contains the implementation of BLE event handling for a Bluetooth Low Energy (BLE)
   device using the Silicon Labs Bluetooth stack. It manages BLE system initialization, connection handling,
   advertising control, and GATT characteristic updates.

   The implementation includes:
   - System boot event: Initializes BLE, retrieves the device address, sets up advertising, and starts advertising.
   - Connection handling: Manages connection establishment, updates connection parameters, and handles disconnections.
   - GATT characteristic status event: Processes client configuration flags for temperature measurement indications.
   - Connection parameters update event: Logs and manages updated connection parameters.

  References:
  - Silicon Labs Bluetooth API documentation
  - GATT Database configuration (gatt_db.h)
  - API Documents
  - Lecture slides
*/
#include "ble.h"
#define INCLUDE_LOG_DEBUG 1
#include "log.h"
#include "src/timers.h"
#include "gatt_db.h"
#include <src/lcd.h>
#include <src/gpio.h>

#define INDICATION_QUEUE_SIZE 10 // Adjust as needed

typedef struct
{
    uint8_t buffer[INDICATION_QUEUE_SIZE];
    uint8_t head;
    uint8_t tail;
    uint8_t count;
} indication_queue_t;

// BLE private data
ble_data_struct_t ble_data;

// Function that returns a pointer to the BLE private data
ble_data_struct_t *getBleDataPtr()
{
    return (&ble_data);
} // getBleDataPtr()

char bleAddressStr[18]; // Format: "XX:XX:XX:XX:XX:XX" + null terminator

// Declare memory for the queue/buffer, and our write and read pointers.
queue_struct_t my_queue[QUEUE_DEPTH]; // the queue
uint32_t wptr = 0;                    // write pointer
uint32_t rptr = 0;                    // read pointer

#if (DEVICE_IS_BLE_SERVER == 0)
typedef enum
{
    WAIT_FOR_PB0_PRESS,
    WAIT_FOR_PB1_PRESS,
    WAIT_FOR_PB1_RELEASE,
    WAIT_FOR_PB0_RELEASE,
    TOGGLE_INDICATION
} button_sequence_state_t;

static button_sequence_state_t button_sequence_state = WAIT_FOR_PB0_PRESS;
static bool indication_enabled = true; // Tracks the indication enable state

// Flex Sensor Service UUID: 39b6b0dc-6ee2-4e77-a665-75d0b64649a3
static const uint8_t flexSensorService_UUID1[16] = {
    0xa3, 0x49, 0x46, 0xb6, 0xd0, 0x75, 0x65, 0xa6,
    0x77, 0x4e, 0xe2, 0x6e, 0xdc, 0xb0, 0xb6, 0x39
};

// Flex Sensor Characteristic UUID: b1082442-5cb6-4d30-9d8c-12094979f6be
static const uint8_t flexSensorChar_UUID1[16] = {
    0xbe, 0xf6, 0x79, 0x49, 0x09, 0x12, 0x8c, 0x9d,
    0x30, 0x4d, 0xb6, 0x5c, 0x42, 0x24, 0x08, 0xb1
};

// Accelerometer Service UUID: aa6321f1-ee79-4f7c-833f-0f6bfcdc0d32
static const uint8_t accelService_UUID1[16] = {
    0x32, 0x0d, 0xdc, 0xfc, 0x6b, 0x0f, 0x3f, 0x83,
    0x7c, 0x4f, 0x79, 0xee, 0xf1, 0x21, 0x63, 0xaa
};

// Accelerometer Characteristic UUID: 5bb27a07-3455-4576-bfb6-f7ae4e45aca9
static const uint8_t accelChar_UUID1[16] = {
    0xa9, 0xac, 0x45, 0x4e, 0xae, 0xf7, 0xb6, 0xbf,
    0x76, 0x45, 0x55, 0x34, 0x07, 0x7a, 0xb2, 0x5b
};
#endif
// ---------------------------------------------------------------------
// Private function used only by this .c file.
// Compute the next ptr value. Given a valid ptr value, compute the next valid
// value of the ptr and return it.
// Isolation of functionality: This defines "how" a pointer advances.
// ---------------------------------------------------------------------
static uint32_t nextPtr(uint32_t ptr)
{
    return ((ptr + 1) % QUEUE_DEPTH);

} // nextPtr()

// ---------------------------------------------------------------------
// This function returns the wptr, rptr, full and empty values, writing
// to memory using the pointer values passed in, same rationale as read_queue()
// The "_" characters are used to disambiguate the global variable names from
// the input parameter names, such that there is no room for the compiler to make a
// mistake in interpreting your intentions.
// ---------------------------------------------------------------------
void get_queue_status(uint32_t *_wptr, uint32_t *_rptr, bool *_full, bool *_empty)
{
    // Return the current values of the pointers and the full/empty flags
    uint32_t temp_wptr = wptr; // Copy the actual values
    uint32_t temp_rptr = rptr;

    *_wptr = temp_wptr; // Return the original values
    *_rptr = temp_rptr;

    *_full = (nextPtr(temp_wptr) == temp_rptr); // Check full condition without modifying pointers
    *_empty = (temp_wptr == temp_rptr);         // Check empty condition

} // get_queue_status()

// ---------------------------------------------------------------------
// Function that computes the number of written entries currently in the queue. If there
// are 3 entries in the queue, it should return 3. If the queue is empty it should
// return 0. If the queue is full it should return either QUEUE_DEPTH if
// USE_ALL_ENTRIES==1 otherwise returns QUEUE_DEPTH-1.
// ---------------------------------------------------------------------
uint32_t get_queue_depth()
{
    bool full, empty;
    uint32_t wptr_temp, rptr_temp;
    get_queue_status(&wptr_temp, &rptr_temp, &full, &empty);
    if (full)
    {
        return (QUEUE_DEPTH - 1); // Full
    }
    else if (empty)
    {
        return 0; // Empty
    }
    else
        return (wptr - rptr + QUEUE_DEPTH) % QUEUE_DEPTH; // Depth calculation

} // get_queue_depth()

// ---------------------------------------------------------------------
// This function writes an entry to the queue if the the queue is not full.
// Input parameter "charHandle" should be written to queue_struct_t element "charHandle".
// Input parameter "bufLength" should be written to queue_struct_t element "bufLength"
// The bytes pointed at by input parameter "buffer" should be written to queue_struct_t element "buffer"
// Returns bool false if successful or true if writing to a full fifo.
// i.e. false means no error, true means an error occurred.
// ---------------------------------------------------------------------
bool write_queue(uint16_t charHandle, uint32_t bufLength, uint8_t *buffer)
{
    // LOG_INFO("Writing to the queue");
    // Range check bufLength
    if (bufLength < MIN_BUFFER_LENGTH || bufLength > MAX_BUFFER_LENGTH)
    {
        return true; // Error: invalid buffer length
    }

    // Check if the queue is full
    bool full, empty;
    uint32_t wptr_temp, rptr_temp;
    get_queue_status(&wptr_temp, &rptr_temp, &full, &empty);

    if (full)
    {
        return true; // Error: queue is full
    }

    // Write data to the queue entry at the current write pointer
    queue_struct_t *current_entry = &my_queue[wptr];
    current_entry->charHandle = charHandle; // Write charHandle
    current_entry->bufLength = bufLength;   // Write bufLength

    // Use memcpy to copy the data into the buffer
    memcpy(current_entry->buffer, buffer, bufLength); // Copy data into queue buffer

    // Advance the write pointer (circular buffer logic)
    wptr = nextPtr(wptr); // Update wptr using nextPtr

    return false; // Successful write

} // write_queue()

// ---------------------------------------------------------------------
// This function reads an entry from the queue, and returns values to the
// caller. The values from the queue entry are returned by writing
// the values to variables declared by the caller, where the caller is passing
// in pointers to charHandle, bufLength and buffer. The caller's code will look like this:
//
//   uint16_t     charHandle;
//   uint32_t     bufLength;
//   uint8_t      buffer[5];
//
//   status = read_queue (&charHandle, &bufLength, &buffer[0]);
//
// *** If the code above doesn't make sense to you, you probably lack the
// necessary prerequisite knowledge to be successful in this course.
//
// Write the values of charHandle, bufLength, and buffer from my_queue[rptr] to
// the memory addresses pointed at by charHandle, bufLength and buffer, like this :
//      *charHandle = <something>;
//      *bufLength  = <something_else>;
//      *buffer     = <something_else_again>; // perhaps memcpy() would be useful?
// ---------------------------------------------------------------------
bool read_queue(uint16_t *charHandle, uint32_t *bufLength, uint8_t *buffer)
{
    // LOG_INFO("Reading from the queue");
    // Check if the queue is empty
    bool full, empty;
    uint32_t wptr_temp, rptr_temp;
    get_queue_status(&wptr_temp, &rptr_temp, &full, &empty);

    if (empty)
    {
        return true; // Error: queue is empty
    }

    // Read data from the queue entry at the current read pointer
    queue_struct_t *current_entry = &my_queue[rptr];
    *charHandle = current_entry->charHandle;           // Retrieve charHandle
    *bufLength = current_entry->bufLength;             // Retrieve bufLength
    memcpy(buffer, current_entry->buffer, *bufLength); // Copy buffer data

    // Advance the read pointer (circular buffer logic)
    rptr = nextPtr(rptr); // Update rptr using nextPtr

    return false; // Successful read

} // read_queue()

// Function to check the server ble device is the required/tragetted one or not
bool is_target_device(bd_addr address)
{
    // Define server_address explicitly with a size of 6
    static const uint8_t server_address[] = SERVER_BT_ADDRESS;

    // Compare bytes
    for (size_t i = 0; i < sizeof(server_address); i++)
    {
        if (address.addr[i] != server_address[i])
        {
            return false;
        }
    }
    return true;
}

// Taken from assignment document to convert the temperature values
int32_t FLOAT_TO_INT32(const uint8_t *buffer_ptr)
{
    uint8_t signByte = 0;
    int32_t mantissa;
    // input data format is:
    // [0] = flags byte, bit[0] = 0 -> Celsius; =1 -> Fahrenheit
    // [3][2][1] = mantissa (2's complement)
    // [4] = exponent (2's complement)
    // BT buffer_ptr[0] has the flags byte
    int8_t exponent = (int8_t)buffer_ptr[4];
    // sign extend the mantissa value if the mantissa is negative
    if (buffer_ptr[3] & 0x80)
    { // msb of [3] is the sign of the mantissa
        signByte = 0xFF;
    }
    mantissa = (int32_t)(buffer_ptr[1] << 0) |
               (buffer_ptr[2] << 8) |
               (buffer_ptr[3] << 16) |
               (signByte << 24);

    // value = 10^exponent * mantissa, pow() returns a double type
    return (int32_t)(pow(10, exponent) * mantissa);
} // FLOAT_TO_INT32

/**
 * @brief Handles Bluetooth Low Energy (BLE) events for both server and client devices.
 *
 * This function processes Bluetooth events for devices acting as either a server or a client.
 * For a server, it handles events related to system boot, connection establishment, connection closure,
 * GATT characteristic status updates, and connection parameters changes. It performs tasks like
 * initializing the LCD, managing advertising, opening and closing connections, and handling GATT characteristics.
 * For a client, it handles events like system boot, scanning for devices, handling scan reports, and opening connections.
 * The function also manages soft timer events for updating the display.
 *
 * @param evt The Bluetooth event message that contains event-specific data and details.
 *
 * @return None
 */
void handle_ble_event(sl_bt_msg_t *evt)
{

// ******************************************************
// Events just for Servers
// ******************************************************
#if (DEVICE_IS_BLE_SERVER == 1)
    sl_status_t status;
    switch (SL_BT_MSG_ID(evt->header))
    {

    case sl_bt_evt_system_boot_id:
        // LCD initalization and initial prints
        displayInit();
        displayPrintf(DISPLAY_ROW_NAME, BLE_DEVICE_TYPE_STRING);
        displayPrintf(DISPLAY_ROW_CONNECTION, BLE_DEVICE_ADVERTISING);
        displayPrintf(DISPLAY_ROW_ASSIGNMENT, A9_STRING);
        displayPrintf(DISPLAY_ROW_TEMPVALUE, "");
        displayPrintf(DISPLAY_ROW_9, "Button Released");

        status = sl_bt_system_get_identity_address(&ble_data.myAddress, &ble_data.myAddressType);
        if (status != SL_STATUS_OK)
        {
            LOG_ERROR("Error getting identity address");
            return;
        }

        displayPrintf(DISPLAY_ROW_BTADDR, "%02X:%02X:%02X:%02X:%02X:%02X",
                      ble_data.myAddress.addr[0], ble_data.myAddress.addr[1],
                      ble_data.myAddress.addr[2], ble_data.myAddress.addr[3],
                      ble_data.myAddress.addr[4], ble_data.myAddress.addr[5]);

        // Create an advertiser set
        status = sl_bt_advertiser_create_set(&ble_data.advertisingSetHandle);
        if (status != SL_STATUS_OK)
        {
            LOG_ERROR("Error creating advertiser");
            return;
        }

        // Set the advertising timing
        status = sl_bt_advertiser_set_timing(
            ble_data.advertisingSetHandle,
            ADV_INTERVAL_MIN, // Min advertising interval
            ADV_INTERVAL_MAX, // Max advertising interval
            ADV_DURATION,     // Duration
            ADV_MAX_EVENTS    // Max events
        );

        if (status != SL_STATUS_OK)
        {
            LOG_ERROR("Error setting advertising timing");
            return;
        }
        // Generate advertising data
        status = sl_bt_legacy_advertiser_generate_data(ble_data.advertisingSetHandle, sl_bt_advertiser_general_discoverable);
        if (status != SL_STATUS_OK)
        {
            LOG_ERROR("Error generating advertising data");
            return;
        }

        // Start advertising
        status = sl_bt_legacy_advertiser_start(ble_data.advertisingSetHandle, sl_bt_legacy_advertiser_connectable);
        if (status != SL_STATUS_OK)
        {
            LOG_ERROR("Error starting advertising");
            return;
        }

        // Enable bonding and increase security
        sl_bt_sm_configure(SM_CONFIG_BONDING_FLAGS, sl_bt_sm_io_capability_displayyesno);
        ble_data.bonding_handle = false;
        sl_bt_sm_delete_bondings();           // Ensure fresh bonding each time
        GPIO_PinOutClear(LED_port, LED0_pin); // Turn off LED0
        GPIO_PinOutClear(LED_port, LED1_pin); // Turn off LED1
        // handle boot event
        break;

    case sl_bt_evt_connection_opened_id:
        // Stop advertising since a connection has been established
        status = sl_bt_advertiser_stop(ble_data.advertisingSetHandle);
        if (status != SL_STATUS_OK)
        {
            LOG_ERROR("Error stopping advertising");
        }

        // LCD update regarding connection
        displayPrintf(DISPLAY_ROW_CONNECTION, BLE_DEVICE_CONNECTED);

        // Set connection parameters using defined macros
        status = sl_bt_connection_set_parameters(
            evt->data.evt_connection_opened.connection,
            MIN_CONN_INTERVAL,
            MAX_CONN_INTERVAL,
            CONN_LATENCY,
            CONN_TIMEOUT,
            PREFERRED_PHY,
            RESERVED_VALUE);

        if (status != SL_STATUS_OK)
        {
            LOG_ERROR("Error setting connection parameters");
        }
        ble_data.connection_open = true;
        ble_data.connection_handle = evt->data.evt_connection_opened.connection;
        // handle open event
        break;

    case sl_bt_evt_sm_confirm_passkey_id:
    {
        uint32_t passkey = evt->data.evt_sm_confirm_passkey.passkey;
        char passkey_str[7];                                          // Passkeys are 6 digits max, +1 for null terminator
        snprintf(passkey_str, sizeof(passkey_str), "%06lu", passkey); // Format as 6-digit zero-padded string

        displayPrintf(DISPLAY_ROW_PASSKEY, passkey_str);
        displayPrintf(DISPLAY_ROW_ACTION, CONFIRM_WITH_PB0);
    }
    break;

    case sl_bt_evt_system_external_signal_id:
        if ((evt->data.evt_system_external_signal.extsignals & PB0_EXT_SIGNAL))
        {
            uint8_t button_state = GPIO_PinInGet(BUTTON_ENABLE_PORT, BUTTON0_PIN) ? 0x00 : 0x01;

            if (ble_data.bonding_handle == false)
            {
                sl_bt_sm_passkey_confirm(ble_data.connection_handle, 1);

                displayPrintf(DISPLAY_ROW_PASSKEY, "");
                displayPrintf(DISPLAY_ROW_ACTION, "");
            }

            if (ble_data.bonding_handle == true && ble_data.connection_open == true)
            {
                // Device is bonded and button is pressed,  and add to database

                // Write to GATT database
                status = sl_bt_gatt_server_write_attribute_value(gattdb_button_state, 0, sizeof(button_state), &button_state);

                if (status != SL_STATUS_OK)
                {
                    LOG_ERROR("Error setting connection parameters");
                }

                // If indications are ENABLED, while checking the connection open and bonding, send BLE Indication
                if (ble_data.ok_to_send_button_indications == true && ble_data.bonding_handle == true)
                {
                    // Check if we can send immediately send or need to enqueue
                    if (ble_data.indication_in_flight == false && (get_queue_depth() == 0))
                    {
                        // Send immidiate data
                        status = sl_bt_gatt_server_send_indication(ble_data.connection_handle, gattdb_button_state, sizeof(button_state), &button_state);
                        if (status == SL_STATUS_OK)
                        {
                            ble_data.indication_in_flight = true;
                        }
                        else
                        {
                            LOG_ERROR("Sending indication fail");
                            // If inidcation fails, write to queue
                            write_queue(gattdb_button_state, sizeof(button_state), &button_state);
                        }
                    }
                    else
                    { // If in flight is true or if the queue has few elements, send it to queue
                        write_queue(gattdb_button_state, sizeof(button_state), &button_state);
                    }
                }
            }
            if (button_state == 0x01)
            {
                displayPrintf(DISPLAY_ROW_9, BUTTON_PRESSED);
            }
            else if (button_state == 0x00)
            {
                displayPrintf(DISPLAY_ROW_9, BUTTON_RELEASED);
            }
        }

        break;

    case sl_bt_evt_sm_confirm_bonding_id:

        // Accept the bonding request
        status = sl_bt_sm_bonding_confirm(evt->data.evt_sm_confirm_bonding.connection, 1); // 1 = accept bonding

        if (status != SL_STATUS_OK)
        {
            LOG_ERROR("Failed to confirm bonding, error:");
        }
        break;

    case sl_bt_evt_sm_bonded_id:
        ble_data.bonding_handle = true;
        displayPrintf(DISPLAY_ROW_CONNECTION, BLE_BONDED);
        break;

    case sl_bt_evt_sm_bonding_failed_id:
        ble_data.bonding_handle = false;
        // displayPrintf(DISPLAY_ROW_CONNECTION, BLE_BONDING_FAILED);
        displayPrintf(DISPLAY_ROW_PASSKEY, "");
        displayPrintf(DISPLAY_ROW_ACTION, "");
        break;

    case sl_bt_evt_connection_closed_id:

        // LCD update regarding connection
        displayPrintf(DISPLAY_ROW_CONNECTION, BLE_DEVICE_ADVERTISING);
        displayPrintf(DISPLAY_ROW_TEMPVALUE, "");
        displayPrintf(DISPLAY_ROW_ACTION, "");
        displayPrintf(DISPLAY_ROW_PASSKEY, "");
        ble_data.connection_open = false;
        ble_data.connection_handle = 0;
        ble_data.ok_to_send_htm_indications = false;
        ble_data.indication_in_flight = false;

        // Restart advertising after disconnection
        status = sl_bt_legacy_advertiser_start(ble_data.advertisingSetHandle, sl_bt_legacy_advertiser_connectable);
        if (status != SL_STATUS_OK)
        {
            LOG_ERROR("Error restarting advertising");
        }
        ble_data.bonding_handle = false;

        GPIO_PinOutClear(LED_port, LED1_pin); // Turn off LED1
        GPIO_PinOutClear(LED_port, LED0_pin); // Turn off LED0
        sl_bt_sm_delete_bondings();           // Ensure fresh bonding each time
        // handle close event
        break;

    case sl_bt_evt_gatt_server_characteristic_status_id:

        // Handle HTM (Health Thermometer Measurement) indications
        if (evt->data.evt_gatt_server_characteristic_status.characteristic == gattdb_temperature_measurement)
        {
            if (evt->data.evt_gatt_server_characteristic_status.status_flags == sl_bt_gatt_server_client_config)
            {
                if (evt->data.evt_gatt_server_characteristic_status.client_config_flags & sl_bt_gatt_server_indication)
                {
                    ble_data.ok_to_send_htm_indications = true;
                    GPIO_PinOutSet(LED_port, LED0_pin); // Turn on LED0
                }
                else
                {
                    ble_data.ok_to_send_htm_indications = false;
                    GPIO_PinOutClear(LED_port, LED0_pin); // Turn off LED0
                    displayPrintf(DISPLAY_ROW_TEMPVALUE, "");
                }
            }
            if (evt->data.evt_gatt_server_characteristic_status.status_flags == sl_bt_gatt_server_confirmation)
            {
                ble_data.indication_in_flight = false; // Allow sending a new indication

                if ((get_queue_depth() > 0) && ble_data.ok_to_send_htm_indications == true && ble_data.connection_open == true && ble_data.bonding_handle == true)
                {
                    // Read from the queue and send indication
                    uint16_t charHandle;
                    uint32_t bufLength;
                    uint8_t buffer[MAX_BUFFER_LENGTH]; // Define a buffer to hold the dequeued data

                    bool read_status = read_queue(&charHandle, &bufLength, buffer);
                    if (!read_status) // Successfully read from the queue
                    {
                        status = sl_bt_gatt_server_send_indication(ble_data.connection_handle, charHandle, bufLength, buffer);
                        if (status == SL_STATUS_OK)
                        {
                            ble_data.indication_in_flight = true;
                        }
                        else
                        {
                            LOG_ERROR("Failed to send indication");
                        }
                    }
                }
            }
        }

        // Handle custom encrypted characteristic "button_state"
        if (evt->data.evt_gatt_server_characteristic_status.characteristic == gattdb_button_state)
        {
            if (evt->data.evt_gatt_server_characteristic_status.status_flags == sl_bt_gatt_server_client_config)
            {
                if (evt->data.evt_gatt_server_characteristic_status.client_config_flags & sl_bt_gatt_server_indication)
                {
                    ble_data.ok_to_send_button_indications = true;
                    GPIO_PinOutSet(LED_port, LED1_pin); // Turn on LED1
                }
                else
                {
                    ble_data.ok_to_send_button_indications = false;
                    GPIO_PinOutClear(LED_port, LED1_pin); // Turn off LED1
                }
            }
            if (evt->data.evt_gatt_server_characteristic_status.status_flags == sl_bt_gatt_server_confirmation)
            {
                ble_data.indication_in_flight = false; // Allow sending a new indication

                if ((get_queue_depth() > 0) && ble_data.ok_to_send_button_indications == true && ble_data.connection_open == true && ble_data.bonding_handle == true)
                {
                    // Read from the queue and send indication
                    uint16_t charHandle;
                    uint32_t bufLength;
                    uint8_t buffer[MAX_BUFFER_LENGTH]; // Define a buffer to hold the dequeued data

                    bool read_status = read_queue(&charHandle, &bufLength, buffer);
                    if (!read_status) // Successfully read from the queue
                    {
                        status = sl_bt_gatt_server_send_indication(ble_data.connection_handle, charHandle, bufLength, buffer);
                        if (status == SL_STATUS_OK)
                        {
                            ble_data.indication_in_flight = true;
                        }
                        else
                        {
                            LOG_ERROR("Failed to send indication");
                        }
                    }
                }
            }
        }
        break;

    case sl_bt_evt_connection_parameters_id:
    { // Commented Log lines for connection parameters value -
      // uint16_t interval = evt->data.evt_connection_parameters.interval;
      // uint16_t latency = evt->data.evt_connection_parameters.latency;
      // uint16_t timeout = evt->data.evt_connection_parameters.timeout;
      // LOG_INFO("Updated Connection Parameters:");
      // LOG_INFO(" - Interval: %d ms", interval * 125 / 100); // Convert from 1.25ms units
      // LOG_INFO(" - Latency: %d", latency);
      // LOG_INFO(" - Supervision Timeout: %d ms", timeout * 10); // Convert from 10ms units
    }
    break;

    case sl_bt_evt_system_soft_timer_id:
        displayUpdate();
        break;
    }
#endif

    // ******************************************************
    // Events just for Client
    // ******************************************************

#if (DEVICE_IS_BLE_SERVER == 0)
    sl_status_t status;
    switch (SL_BT_MSG_ID(evt->header))
    {
    case sl_bt_evt_system_boot_id:

        displayInit();
        displayPrintf(DISPLAY_ROW_NAME, BLE_DEVICE_TYPE_STRING);
        displayPrintf(DISPLAY_ROW_CONNECTION, BLE_DEVICE_DISCOVERING);
        displayPrintf(DISPLAY_ROW_ASSIGNMENT, A9_STRING);
        displayPrintf(DISPLAY_ROW_TEMPVALUE, "");

        status = sl_bt_system_get_identity_address(&ble_data.myAddress, &ble_data.myAddressType);
        if (status != SL_STATUS_OK)
        {
            LOG_ERROR("Error getting identity address");
            return;
        }
        // // Print BLE address on the LCD
        // displayPrintf(DISPLAY_ROW_BTADDR, bleAddressStr);
        displayPrintf(DISPLAY_ROW_BTADDR, "%02X:%02X:%02X:%02X:%02X:%02X",
                      ble_data.myAddress.addr[0], ble_data.myAddress.addr[1],
                      ble_data.myAddress.addr[2], ble_data.myAddress.addr[3],
                      ble_data.myAddress.addr[4], ble_data.myAddress.addr[5]);

        // Set connection parameters (example values)
        status = sl_bt_connection_set_default_parameters(
            MIN_CONN_INTERVAL,
            MAX_CONN_INTERVAL,
            CONN_LATENCY,
            CONN_TIMEOUT,
            PREFERRED_PHY,
            RESERVED_VALUE);

        if (status != SL_STATUS_OK)
        {
            LOG_ERROR("Error setting connection parameters");
        }

        // Set the scanner parameters using the defined macros
        status = sl_bt_scanner_set_parameters(
            SCANNER_POLICY, // Scanner filter policy
            SCAN_INTERVAL,  // Scan interval
            SCAN_WINDOW     // Scan window
        );

        if (status != SL_STATUS_OK)
        {
            LOG_ERROR("Error setting the parameters for scanning");
        }

        // Start scanning with the specified PHY and discovery mode using the defined macros
        status = sl_bt_scanner_start(
            SCANNING_PHY,  // Scanning PHY to be used
            DISCOVERY_MODE // Discovery mode for scanning
        );

        if (status != SL_STATUS_OK)
        {
            LOG_ERROR("Error starting scanner");
        }
        // Enable bonding and increase security
        sl_bt_sm_configure(SM_CONFIG_BONDING_FLAGS, sl_bt_sm_io_capability_displayyesno);
        ble_data.bonding_handle = false;
        sl_bt_sm_delete_bondings();           // Ensure fresh bonding each time
        GPIO_PinOutClear(LED_port, LED0_pin); // Turn off LED0
        GPIO_PinOutClear(LED_port, LED1_pin); // Turn off LED1
        break;

    case sl_bt_evt_scanner_legacy_advertisement_report_id:
        if (is_target_device(evt->data.evt_scanner_scan_report.address)) // Check address
        {
            // Check if the address type is Public Address (0x00)
            if ((evt->data.evt_scanner_scan_report.address_type == PUBLIC_DEVICE_ADDRESS)) // Check address type
            {
                // Save the scanned device address before connection
                memcpy(ble_data.connectedDeviceAddress.addr, evt->data.evt_scanner_scan_report.address.addr, sizeof(bd_addr));

                // 0x00 corresponds to the Public Device Address as per Bluetooth specification

                // Check if the advertisement is both connectable and scannable.
                // - Bit 0 (0x01): Set to 1 if the advertisement is scannable (can be discovered).
                // - Bit 1 (0x02): Set to 1 if the advertisement is connectable (can initiate a connection).

                // The bitwise AND operation with 0x03 checks if both bit 0 and bit 1 are set.
                if ((evt->data.evt_scanner_legacy_advertisement_report.event_flags & ADVERTISEMENT_CONNECTABLE_SCANNABLE) == ADVERTISEMENT_CONNECTABLE_SCANNABLE) // Check if it is connectable and scannble
                {
                    status = sl_bt_scanner_stop();

                    if (status != SL_STATUS_OK)
                    {
                        LOG_ERROR("Error stopping scanner: 0x%lx", status);
                    }
                    status = sl_bt_connection_open(evt->data.evt_scanner_scan_report.address,
                                                   evt->data.evt_scanner_scan_report.address_type,
                                                   sl_bt_gap_phy_1m,
                                                   NULL);
                    if (status != SL_STATUS_OK)
                    {
                        LOG_ERROR("Error opening connection: 0x%lx", status);
                    }
                }
            }
        }
        break;

    case sl_bt_evt_connection_opened_id:
        ble_data.connection_handle = evt->data.evt_connection_opened.connection;
        // LCD update regarding connection

        displayPrintf(DISPLAY_ROW_CONNECTION, BLE_DEVICE_CONNECTED);
        // Display scanned device address
        displayPrintf(DISPLAY_ROW_BTADDR2, "%02X:%02X:%02X:%02X:%02X:%02X",
                      ble_data.connectedDeviceAddress.addr[0],
                      ble_data.connectedDeviceAddress.addr[1],
                      ble_data.connectedDeviceAddress.addr[2],
                      ble_data.connectedDeviceAddress.addr[3],
                      ble_data.connectedDeviceAddress.addr[4],
                      ble_data.connectedDeviceAddress.addr[5]);

        break;

    case sl_bt_evt_connection_closed_id:
        displayPrintf(DISPLAY_ROW_CONNECTION, BLE_DEVICE_DISCOVERING);
        displayPrintf(DISPLAY_ROW_TEMPVALUE, "");
        displayPrintf(DISPLAY_ROW_BTADDR2, "");
        displayPrintf(DISPLAY_ROW_ACTION, "");
        displayPrintf(DISPLAY_ROW_PASSKEY, "");
        ble_data.connection_open = false;
        ble_data.bonding_handle = false;
        // Start scanning with the specified PHY and discovery mode using the defined macros
        status = sl_bt_scanner_start(
            SCANNING_PHY,  // Scanning PHY to be used
            DISCOVERY_MODE // Discovery mode for scanning
        );
        if (status != SL_STATUS_OK)
        {
            LOG_ERROR("Error starting scanner");
        }
        GPIO_PinOutClear(LED_port, LED1_pin); // Turn off LED1
        GPIO_PinOutClear(LED_port, LED0_pin); // Turn off LED0
        sl_bt_sm_delete_bondings();           // Ensure fresh bonding each time
        break;

    case sl_bt_evt_system_external_signal_id:

        // static bool sequence_active = false; // Track if sequence is active

        if ((ble_data.bonding_handle == false) && (evt->data.evt_system_external_signal.extsignals & PB0_EXT_SIGNAL))
        {

            sl_bt_sm_passkey_confirm(ble_data.connection_handle, 1);
            displayPrintf(DISPLAY_ROW_PASSKEY, "");
            displayPrintf(DISPLAY_ROW_ACTION, "");
        }
        uint8_t button0_state = GPIO_PinInGet(BUTTON_ENABLE_PORT, BUTTON0_PIN) ? BUTTON_RELEASED_VALUE : BUTTON_PRESSED_VALUE;
        uint8_t button1_state = GPIO_PinInGet(BUTTON_ENABLE_PORT, BUTTON1_PIN) ? BUTTON_RELEASED_VALUE : BUTTON_PRESSED_VALUE;

        if ((evt->data.evt_system_external_signal.extsignals & PB1_EXT_SIGNAL) && (button0_state == BUTTON_RELEASED_VALUE))
        {

            sl_bt_gatt_read_characteristic_value(ble_data.connection_handle, gattdb_flex_data);
            sl_bt_gatt_read_characteristic_value(ble_data.connection_handle, gattdb_accelerometer_data);
        }

        if ((evt->data.evt_system_external_signal.extsignals & PB0_EXT_SIGNAL) || (evt->data.evt_system_external_signal.extsignals & PB1_EXT_SIGNAL))
        {

            switch (button_sequence_state)
            {
            case WAIT_FOR_PB0_PRESS:
                if (button0_state == BUTTON_PRESSED_VALUE)
                {

                    button_sequence_state = WAIT_FOR_PB1_PRESS;
                }
                break;

            case WAIT_FOR_PB1_PRESS:
                if (button1_state == BUTTON_PRESSED_VALUE)
                {
                    if (button0_state == BUTTON_RELEASED_VALUE)
                    {

                        button_sequence_state = WAIT_FOR_PB0_PRESS;
                        break;
                    }

                    button_sequence_state = WAIT_FOR_PB1_RELEASE;
                }
                break;

            case WAIT_FOR_PB1_RELEASE:
                if (button1_state == BUTTON_RELEASED_VALUE)
                {
                    if (button0_state == BUTTON_RELEASED_VALUE)
                    {

                        button_sequence_state = WAIT_FOR_PB0_PRESS;
                        break;
                    }

                    button_sequence_state = WAIT_FOR_PB0_RELEASE;
                }
                break;

            case WAIT_FOR_PB0_RELEASE:
                if (button1_state == BUTTON_PRESSED_VALUE)
                {

                    button_sequence_state = WAIT_FOR_PB0_PRESS;
                    break;
                }
                if (button0_state == BUTTON_RELEASED_VALUE)
                {

                    // Toggle the characteristic indication enable state
                    indication_enabled = !indication_enabled;
                    if (indication_enabled)
                      {
                        sl_status_t status = sl_bt_gatt_set_characteristic_notification(
                            getBleDataPtr()->connection_handle,
                            getBleDataPtr()->flex_characteristic_handle,
                            sl_bt_gatt_indication);
                        if (status != SL_STATUS_OK)
                          {
                            LOG_ERROR("Error setting notifications");
                            return;
                          }
                         status = sl_bt_gatt_set_characteristic_notification(
                            getBleDataPtr()->connection_handle,
                            getBleDataPtr()->accel_characteristic_handle,
                            sl_bt_gatt_indication);
                        if (status != SL_STATUS_OK)
                          {
                            LOG_ERROR("Error setting notifications");
                            return;
                          }
                      }
                    else
                      {
                        sl_status_t status = sl_bt_gatt_set_characteristic_notification(
                            getBleDataPtr()->connection_handle,
                            getBleDataPtr()->flex_characteristic_handle,
                            sl_bt_gatt_disable);
                        if (status != SL_STATUS_OK)
                          {
                            LOG_ERROR("Error setting notifications");
                            return;
                          }
                         status = sl_bt_gatt_set_characteristic_notification(
                            getBleDataPtr()->connection_handle,
                            getBleDataPtr()->accel_characteristic_handle,
                            sl_bt_gatt_disable);
                        if (status != SL_STATUS_OK)
                          {
                            LOG_ERROR("Error setting notifications");
                            return;
                          }
                      }

                    // Reset state machine for next sequence
                    button_sequence_state = WAIT_FOR_PB0_PRESS;
                }
                break;

            default:

                button_sequence_state = WAIT_FOR_PB0_PRESS;
                break;
            }
        }
        break;

    case sl_bt_evt_sm_confirm_passkey_id:
    {

        uint32_t passkey = evt->data.evt_sm_confirm_passkey.passkey;
        char passkey_str[7];                                          // Passkeys are 6 digits max, +1 for null terminator
        snprintf(passkey_str, sizeof(passkey_str), "%06lu", passkey); // Format as 6-digit zero-padded string

        displayPrintf(DISPLAY_ROW_PASSKEY, passkey_str);
        displayPrintf(DISPLAY_ROW_ACTION, CONFIRM_WITH_PB0);
    }
    break;

    case sl_bt_evt_sm_confirm_bonding_id:

        // Accept the bonding request
        status = sl_bt_sm_bonding_confirm(evt->data.evt_sm_confirm_bonding.connection, 1); // 1 = accept bonding

        if (status != SL_STATUS_OK)
        {
            LOG_ERROR("Failed to confirm bonding, error:");
        }
        break;

    case sl_bt_evt_sm_bonded_id:
        ble_data.bonding_handle = true;
        displayPrintf(DISPLAY_ROW_CONNECTION, BLE_BONDED);
        break;

    case sl_bt_evt_sm_bonding_failed_id:
        ble_data.bonding_handle = false;
        // displayPrintf(DISPLAY_ROW_CONNECTION, BLE_BONDING_FAILED);
        displayPrintf(DISPLAY_ROW_PASSKEY, "");
        displayPrintf(DISPLAY_ROW_ACTION, "");
        break;

    case sl_bt_evt_gatt_service_id:
      {
        uint8_t *uuid_data = evt->data.evt_gatt_service.uuid.data;

        if (memcmp(uuid_data, flexSensorService_UUID1, 16) == 0)
          {
            ble_data.flex_service_handle = evt->data.evt_gatt_service.service;
          }


        if (memcmp(uuid_data, accelService_UUID1, 16) == 0)
          {
            ble_data.accel_service_handle = evt->data.evt_gatt_service.service;
          }
      }
    break;

    case sl_bt_evt_gatt_characteristic_id:
      {
        uint8_t *uuid_data = evt->data.evt_gatt_characteristic.uuid.data;

        if (memcmp(uuid_data, flexSensorChar_UUID1, 16) == 0)
          {
            ble_data.flex_characteristic_handle = evt->data.evt_gatt_characteristic.characteristic;
          }

        if (memcmp(uuid_data, accelChar_UUID1, 16) == 0)
          {
            ble_data.accel_characteristic_handle = evt->data.evt_gatt_characteristic.characteristic;
          }
      }
    break;

    case sl_bt_evt_gatt_characteristic_value_id:

        if (evt->data.evt_gatt_characteristic_value.characteristic == ble_data.flex_characteristic_handle)
        {

            uint8_t *value = evt->data.evt_gatt_characteristic_value.value.data;

            uint8_t angle = value[0];
            displayPrintf(DISPLAY_ROW_9, "Flex Angle:%dDeg",angle);
            if(angle == 0){
                displayPrintf(DISPLAY_ROW_11, "Posture:GOOD");
            }
            else{
                buzzer_on();
                timerWaitUs_irq(500000);
                buzzer_off();
                displayPrintf(DISPLAY_ROW_11, "Posture:BAD");
            }
        }

        if (evt->data.evt_gatt_characteristic_value.characteristic == ble_data.accel_characteristic_handle)
          {

            uint8_t *value = evt->data.evt_gatt_characteristic_value.value.data;

            uint8_t tiltCount = value[0];
            displayPrintf(DISPLAY_ROW_10, "Tilt Count:%d", tiltCount);
          }

        if (evt->data.evt_gatt_characteristic_value.att_opcode == sl_bt_gatt_read_response)
        {

            if (evt->data.evt_gatt_characteristic_value.characteristic == ble_data.flex_characteristic_handle)
              {
                uint8_t *value = evt->data.evt_gatt_characteristic_value.value.data;
                uint8_t angle = value[0];
                displayPrintf(DISPLAY_ROW_9, "Flex Angle:%dDeg",angle);
              }
            if (evt->data.evt_gatt_characteristic_value.characteristic == ble_data.accel_characteristic_handle)
              {
                uint8_t *value = evt->data.evt_gatt_characteristic_value.value.data;
                uint8_t tiltCount = value[0];
                displayPrintf(DISPLAY_ROW_10, "Tilt Count:%d", tiltCount);
              }
        }
        status = sl_bt_gatt_send_characteristic_confirmation(evt->data.evt_gatt_characteristic_value.connection);

        break;

    case sl_bt_evt_gatt_procedure_completed_id:
        // Check for encryption error and handle bonding
        if (evt->data.evt_gatt_procedure_completed.result == SL_STATUS_BT_ATT_INSUFFICIENT_ENCRYPTION)
        {
            sl_bt_sm_increase_security(ble_data.connection_handle);
        }
        break;

    case sl_bt_evt_system_soft_timer_id:
        displayUpdate();
        break;

    } // end - switch

#endif

} // handle_ble_event()
