#ifndef ED_POTENTIOMETER_H
#define ED_POTENTIOMETER_H

#define ED_POTENTIOMETER_ADC_PIN A0

#define ED_POTENTIOMETER_RAW_MAX 1023
#define ED_POTENTIOMETER_RAW_MIN 0

#define ED_POTENTIOMETER_VOLTAGE_MAX 5000
#define ED_POTENTIOMETER_VOLTAGE_MIN 0

void ed_potentiometer_setup();
void ed_potentiometer_loop();

int ed_potentiometer_get_raw();
int ed_potentiometer_get_voltage();

#endif