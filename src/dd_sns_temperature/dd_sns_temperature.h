#ifndef DD_SNS_TEMPERATURE_H
#define DD_SNS_TEMPERATURE_H

// Temperature sensor driver – uses a potentiometer (ed_potentiometer) as an
// analog temperature simulator.  Maps 0–5000 mV → 0–100 °C linearly.

#define DD_SNS_TEMPERATURE_MIN      0    // °C
#define DD_SNS_TEMPERATURE_MAX    100    // °C
#define DD_SNS_TEMPERATURE_CENTER  50    // °C  (mid-range reference)

void dd_sns_temperature_setup();
void dd_sns_temperature_loop();   // call from acquisition task (every 50 ms)

// Mutex-protected getters – safe to call from any FreeRTOS task
int  dd_sns_temperature_get_raw();       // ADC counts,  0..1023
int  dd_sns_temperature_get_voltage();   // millivolts,  0..5000
int  dd_sns_temperature_get_celsius();   // temperature, 0..100 °C

#endif
