
/***********************************************************************
 * @file      scheduler.h
 * @version   0.1
 * @brief     Function header/interface file.
 *
 * @author    Damini Gowda, damini.gowda@colorado.edu
 * @date      Feb 1, 2023
 *
 *
 * @institution University of Colorado Boulder (UCB)
 * @course      ECEN 5823-001: IoT Embedded Firmware
 * @instructor  Chris Choi
 *
 * @assignment Assignment 3 - Si7021 and Load Power Management Part 1
 * @due
 *
 * @resources None
 *
 */

#ifndef SRC_SCHEDULER_H_
#define SRC_SCHEDULER_H_

#include "sl_status.h"
#include "src/i2c.h"
#include "sl_power_manager.h"
#include "src/ble.h"
#include <stdbool.h>
#include "em_core.h"
#include "sl_bt_api.h"


#define MAX_EVENTS 4

typedef enum{
  EVENT_NONE,
  EVENT_LETIMER_UF,
  EVENT_LETIMER_COMP1,
  EVENT_I2CTransfer_Done,
  EVENT_BLEConnectionClose,
  EVENT_PB0
}Events_t;

#if (DEVICE_IS_BLE_SERVER == 1)

typedef enum{
  STATE_IDLE,
  STATE_SENSOROn,
  STATE_I2CWrite,
  STATE_I2CRead,
  STATE_GETResult
}States_t;

#else

typedef enum uint32_t {
  STATE_discoverService,
  STATE_discoverChar,
  STATE_setIndication,
  STATE_getIndication,
  STATE_startScanning,
}State_t;

// Health Thermometer service UUID defined by Bluetooth SIG
static const uint8_t healthThermo_ID[2] = { 0x09, 0x18 };
// Temperature Measurement characteristic UUID defined by Bluetooth SIG
static const uint8_t tempMeasure_ID[2] = { 0x1c, 0x2a };

#endif

void schedulerSetEventUF(void);
void schedulerSetEventCOMP1(void);
void schedulerSetEventI2CDone(void);
void schedulerSetEventBleConnectionClose(void);
void schedulerSetEventPB0(void);
Events_t getNextEvent(void);
bool stateMachineTemperatureRead(sl_bt_msg_t *evt);
void discovery_state_machine(sl_bt_msg_t *evt);

#endif /* SRC_SCHEDULER_H_ */
