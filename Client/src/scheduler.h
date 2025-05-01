#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>
#include "ble_device_type.h"
#include "sl_bt_api.h"
#include "stdbool.h"

// Define event bit masks
#define EVT_LETIMER0_UF (1 << 0)
#define EVT_I2C_WAIT (1 << 1)
#define EVT_TransferDone (1 << 3)

// Scheduler Init
void schedulerInit(void);
// Scheduler function to set UF flag
void schedulerSetEventUF(void);
// Scheduler to set com1 triggered flag when called for I2C wait
void schedulerSetEventI2CWait(void);
// Scheduler to set com1 triggered flag when called for Transfer Done
void schedulerSetEventTransferDone(void);
// Function to get next event
uint32_t getNextEvent(void);
// I2C state machine function
void I2CStateMachine(sl_bt_msg_t *);
// Client state machine
void discovery_state_machine(sl_bt_msg_t *evt);

#endif // SCHEDULER_H