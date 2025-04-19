/***********************************************************************
 * @file      ble.h
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

#ifndef SRC_BLE_H_
#define SRC_BLE_H_

#include "ble_device_type.h"
#include "log.h"
#include "lcd.h"
#include "gatt_db.h"
#include "i2c.h"
#include "irq.h"
#include <math.h>
#include <stdbool.h>
#include "sl_bt_api.h"
#include "scheduler.h"

#define UINT8_TO_BITSTREAM(p, n)        { *(p)++ = (uint8_t)(n); }
#define UINT32_TO_BITSTREAM(p, n)       { *(p)++ = (uint8_t)(n); *(p)++ = (uint8_t)((n) >> 8); \
                                          *(p)++ = (uint8_t)((n) >> 16); *(p)++ = (uint8_t)((n) >> 24); }
#define UINT32_TO_FLOAT(m, e)           (((uint32_t)(m) & 0x00FFFFFFU) | ((uint32_t)(e) << 24))

typedef struct {
 // values that are common to servers and clients
 bd_addr myAddress;
 uint8_t connectionHandle;
 // values unique for server
 // The advertising set handle allocated from Bluetooth stack.
 uint8_t advertisingSetHandle;
 // Connection current state
 bool connection_open;               // true when connection is open
 bool ok_to_send_htm_connections;    // true when client enabled indications
 bool ok_to_send_bs_connections;    // true when client enabled indications
 bool indication_in_flight;          // true when an indication is in flight
 bool bonded;
 bool expecting_passkey_confirmation;

 // values unique for client
  uint8_t myAddressType;
  uint8_t packetType;
  uint32_t serviceHandle;
  uint16_t characteristicHandle;
} ble_data_struct_t;

ble_data_struct_t*  get_ble_data_struct(void);
void handle_ble_event(sl_bt_msg_t *evt);
void send_next_indication(uint8_t button_state);
void enqueue_indication(uint8_t value);
void send_temp_ble(void);

#endif /* SRC_BLE_H_ */
