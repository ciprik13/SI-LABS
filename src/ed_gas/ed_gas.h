#ifndef ED_GAS_H
#define ED_GAS_H

#include <Arduino.h>

// ===========================================================================
// Elementary driver – Analog gas sensor (MQ-2 / MQ-5 / similar).
//
// Reads raw ADC from ED_GAS_ADC_PIN (A1 by default).
// Converts to millivolts and to a 0–100 % concentration level
// (linear mapping of the full ADC range – sufficient for lab purposes).
//
// Hardware connection:
//   Sensor AOUT (AO) → Arduino A1
//   Sensor VCC       → 5 V
//   Sensor GND       → GND
//   (Sensor DOUT/DO  – not used)
//
// To use a different pin: change ED_GAS_ADC_PIN below.
// ===========================================================================
#define ED_GAS_ADC_PIN  A1

#define ED_GAS_RAW_MIN        0      // ADC counts
#define ED_GAS_RAW_MAX     1023      // ADC counts

#define ED_GAS_VOLTAGE_MIN    0      // mV
#define ED_GAS_VOLTAGE_MAX 5000      // mV

void ed_gas_setup();
void ed_gas_loop();            // call from acquisition task every 50 ms

int  ed_gas_get_raw();         // ADC counts  0..1023
int  ed_gas_get_voltage();     // millivolts  0..5000
int  ed_gas_get_percent();     // 0..100 %  (linear full-scale)

#endif // ED_GAS_H
