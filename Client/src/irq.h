// irq.h

#ifndef IRQ_H
#define IRQ_H
#include "stdint.h"

// IRQ Handler Function
void LETIMER0_IRQHandler(void);
// Millisecond logger function
uint32_t letimerMilliseconds(void);
// GPIO EVEN Interrupt Handler (handles PB0 press)
void GPIO_EVEN_IRQHandler(void);
// GPIO EVEN Interrupt Handler (handles PB1 press)
void GPIO_ODD_IRQHandler(void);
#endif // IRQ_H
