#ifndef BLE_H_
#define BLE_H_

#include "stdbool.h"
#include "stdint.h"
#include "ble_device_type.h"
#include "sl_bt_api.h"
#include <math.h>
#include <src/gpio.h>
#include <string.h> // for memcpy()

#define UINT8_TO_BITSTREAM(p, n) \
    {                            \
        *(p)++ = (uint8_t)(n);   \
    }
#define UINT32_TO_BITSTREAM(p, n)      \
    {                                  \
        *(p)++ = (uint8_t)(n);         \
        *(p)++ = (uint8_t)((n) >> 8);  \
        *(p)++ = (uint8_t)((n) >> 16); \
        *(p)++ = (uint8_t)((n) >> 24); \
    }
#define INT32_TO_FLOAT(m, e) ((int32_t)(((uint32_t)m) & 0x00FFFFFFU) | (((uint32_t)e) << 24))

#define BLE_DEVICE_ADVERTISING "Advertising"
#define BLE_DEVICE_CONNECTED "Connected"
#define A9_STRING "A9"
#define BLE_DEVICE_DISCOVERING "Discovering"
#define BLE_HANDLING_INDICATIONS "Handling Indications"
#define BLE_BONDING_FAILED "Bonding Failed"
#define BLE_BONDED "Bonded"
#define BUTTON_PRESSED "Button Pressed"
#define BUTTON_RELEASED "Button Released"
#define CONFIRM_WITH_PB0 "Confirm with PB0"

// Advertising timing intervals (in units of 0.625 ms)
#define ADV_INTERVAL_MIN 400 // 400 * 0.625 ms = 250 ms
#define ADV_INTERVAL_MAX 400 // 400 * 0.625 ms = 250 ms
// Advertising duration (0 = infinite)
#define ADV_DURATION 0
// Maximum advertising events (0 = unlimited)
#define ADV_MAX_EVENTS 0

// Security Manager configuration flags
#define SM_CONFIG_BONDING_FLAGS 0x0F // Enable bonding and secure connections

// Connection interval (in 1.25ms units)
#define MIN_CONN_INTERVAL 60 // 75ms (60 * 1.25ms)
#define MAX_CONN_INTERVAL 60 // 75ms (60 * 1.25ms)

// Connection latency
#define CONN_LATENCY 4 // Number of connection events to skip
// Supervision timeout (in 10ms units)
#define CONN_TIMEOUT 80 // 800ms (80 * 10ms)
// Preferred PHY (0 = no preference)
#define PREFERRED_PHY 0
// Reserved value
#define RESERVED_VALUE 6.4

// Define macros for scan interval and scan window
#define SCAN_INTERVAL 80    // Scan interval (80 * 0.625ms = 50ms)
#define SCAN_WINDOW 40      // Scan window (40 * 0.625ms = 25ms)
#define SCANNER_POLICY 0x00 // Scanner filter policy: 0x00 = No filter (accept all advertisements)
// Define macros for scanning PHY and discovery mode
#define SCANNING_PHY 0x1   // Scanning PHY: 0x1 = 1M PHY, 0x4 = Coded PHY, 0x5 = Both 1M and Coded PHY
#define DISCOVERY_MODE 0x1 // Discovery mode: 0x0 = Limited discoverable devices,
                           // 0x1 = Both limited and general discoverable devices,
                           // 0x2 = Non-discoverable, limited, and general discoverable devices

// BLE Data Structure, save all of our private BT data in here.
// Modern C (circa 2021 does it this way)
// typedef ble_data_struct_t is referred to as an anonymous struct definition

typedef struct
{
    // values that are common to servers and clients
    bd_addr myAddress;
    uint8_t myAddressType;
    bd_addr connectedDeviceAddress;

    // values unique for server
    uint8_t advertisingSetHandle;
    bool connection_open;            // true when in an open connection
    bool ok_to_send_htm_indications; // true when client enabled indications
    bool indication_in_flight;       // true when an indication is in-flight
    uint8_t connection_handle;       // Store active connection handle
    bool ok_to_send_button_indications;
    bool bonding_handle;
    // values unique for client
    uint32_t flex_characteristic_handle;    // Handle for a specific characteristic
    uint32_t flex_service_handle;           // Handle for a specific service
    uint32_t accel_characteristic_handle; // Handle for a specific characteristic
    uint32_t accel_service_handle;        // Handle for a specific service

} ble_data_struct_t;

// This is the number of entries in the queue. Please leave
// this value set to 16.
#define QUEUE_DEPTH (16)

#define MAX_BUFFER_LENGTH (5)
#define MIN_BUFFER_LENGTH (1)
#define BUTTON_RELEASED_VALUE 0x00
#define BUTTON_PRESSED_VALUE 0x01
// Define macros for Bluetooth address types
#define PUBLIC_DEVICE_ADDRESS 0x00
// Define macros for advertisement event flags
#define ADVERTISEMENT_SCANNABLE 0x01
#define ADVERTISEMENT_CONNECTABLE 0x02
#define ADVERTISEMENT_CONNECTABLE_SCANNABLE (ADVERTISEMENT_SCANNABLE | ADVERTISEMENT_CONNECTABLE)

typedef struct
{

    uint16_t charHandle;               // GATT DB handle from gatt_db.h
    uint32_t bufLength;                // Number of bytes written to field buffer[5]
    uint8_t buffer[MAX_BUFFER_LENGTH]; // The actual data buffer for the indication,
                                       //   need 5-bytes for HTM and 1-byte for button_state.
                                       //   For testing, test lengths 1 through 5,
                                       //   a length of 0 shall be considered an
                                       //   error, as well as lengths > 5

} queue_struct_t;

// function prototypes
ble_data_struct_t *getBleDataPtr(void);  // Getting Ble private data
void handle_ble_event(sl_bt_msg_t *evt); // Handle bluetooth stack events case statements

bool is_target_device(bd_addr address);            // Function to check the server ble device is the required/tragetted one or not
int32_t FLOAT_TO_INT32(const uint8_t *buffer_ptr); // Function to convert temperature value received from BLE

void reset_queue(void); // Resets the queue to its initial state

bool write_queue(uint16_t charHandle, uint32_t bufLength, uint8_t *buffer); // Writes data to the queue, returns true on success

bool read_queue(uint16_t *charHandle, uint32_t *bufLength, uint8_t *buffer); // Reads data from the queue, returns true on success

void get_queue_status(uint32_t *wptr, uint32_t *rptr, bool *full, bool *empty); // Retrieves the queue status (write pointer, read pointer, full/empty flags)

uint32_t get_queue_depth(void); // Returns the current depth (number of elements) in the queue

#endif // BLE_H_
