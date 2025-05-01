/***********************************************************************
 * @file      adc.c
 * @version   0.1
 * @brief     Function header/interface file.
 *
 * @author    Damini Gowda, damini.gowda@colorado.edu
 * @date      April 10, 2025
 *
 *
 * @institution University of Colorado Boulder (UCB)
 * @course      ECEN 5823-001: IoT Embedded Firmware
 * @instructor  Chris Choi
 *
 */
#define INCLUDE_LOG_DEBUG     1
#include "log.h"
#include "src/adc.h"

// Init to max ADC clock for Series 1
#define adcFreq   32768

#define NUM_INPUTS  1
#define CHANGE_THRESHOLD_MV 150  // Minimum difference to trigger new event

uint32_t input;
uint32_t input;             // Current voltage in mV
//static uint32_t lastInput = 0;  // Previous voltage in mV
//static uint8_t lastEvent = 0xFF;  // 0 for 0°, 1 for 45°, 2 for 90°, 0xFF for none

void initADC (void)
{

  // Declare init structs
  ADC_Init_TypeDef init = ADC_INIT_DEFAULT;
  ADC_InitScan_TypeDef initScan = ADC_INITSCAN_DEFAULT;

  // Modify init structs
  init.prescale   = ADC_PrescaleCalc(adcFreq, 0);
  init.timebase = ADC_TimebaseCalc(0);

  initScan.diff       = 0;            // single ended
  initScan.reference  = adcRef2V5;    // internal 2.5V reference
  initScan.resolution = adcRes12Bit;  // 12-bit resolution
  initScan.acqTime    = adcAcqTime16;  // set acquisition time to meet minimum requirement
  initScan.fifoOverwrite = true;      // FIFO overflow overwrites old data

  // Select ADC input. See README for corresponding EXP header pin.
  // *Note that internal channels are unavailable in ADC scan mode
  ADC_ScanSingleEndedInputAdd(&initScan, adcScanInputGroup0, adcPosSelAPORT2XCH9);
  //ADC_ScanSingleEndedInputAdd(&initScan, adcScanInputGroup1, adcPosSelAPORT2YCH10);

  // Set scan data valid level (DVL) to 2
  ADC0->SCANCTRLX |= (NUM_INPUTS - 1) << _ADC_SCANCTRLX_DVL_SHIFT;

  // Clear ADC Scan fifo
  ADC0->SCANFIFOCLEAR = ADC_SCANFIFOCLEAR_SCANFIFOCLEAR;

  // Initialize ADC and Scan
  ADC_Init(ADC0, &init);
  ADC_InitScan(ADC0, &initScan);

  // Enable Scan interrupts
  ADC_IntEnable(ADC0, ADC_IEN_SCAN);

  // Enable ADC interrupts
  NVIC_ClearPendingIRQ(ADC0_IRQn);
  NVIC_EnableIRQ(ADC0_IRQn);
}

void ADC0_IRQHandler(void)
{
  uint32_t data,id;
  uint8_t currentEvent;

  // Check if scan data is ready (optional safety check)
  if (ADC_IntGet(ADC0) & ADC_IEN_SCAN)
    {
      // Read data and input ID from scan FIFO
      data = ADC_DataIdScanGet(ADC0, &id);

      // Convert to millivolts using 2.5V reference (12-bit ADC: 4096 steps)
      input = (data * 2500) / 4096;

      // Classify voltage into bend ranges
      if (input <= 1400) {
          currentEvent = 0; // 0°
      } else if (input <= 1550) {
          currentEvent = 1; // 45°
      } else if (input <= 1700) {
          currentEvent = 2; // 90°
      }
      else{
          currentEvent = 0;
      }

      switch (currentEvent) {
        case 0: schedulerSetEvent0(); break;
        case 1: schedulerSetEvent45(); break;
        case 2: schedulerSetEvent90(); break;
      }

      // Clear the interrupt flag
      ADC_IntClear(ADC0, ADC_IF_SCAN);

    }
}
