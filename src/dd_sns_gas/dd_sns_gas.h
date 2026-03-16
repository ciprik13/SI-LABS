#ifndef DD_SNS_GAS_H
#define DD_SNS_GAS_H

// ===========================================================================
// Device driver – Analog gas sensor (MQ-2 / MQ-5, pin A1).
// Wraps ed_gas with a FreeRTOS mutex so getters are safe from any task.
// ===========================================================================

void dd_sns_gas_setup();
void dd_sns_gas_loop();         // call from acquisition task (every 50 ms)

// Mutex-protected getters
int  dd_sns_gas_get_raw();      // ADC counts  0..1023
int  dd_sns_gas_get_voltage();  // millivolts  0..5000
int  dd_sns_gas_get_percent();  // 0..100 %

#endif // DD_SNS_GAS_H
