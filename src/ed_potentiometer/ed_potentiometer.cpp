#include "ed_potentiometer.h"
#include "Arduino.h"

int ed_potentiometer_value_raw = 0;
int ed_potentiometer_value_voltage = 0;

void ed_potentiometer_setup() {

}

void ed_potentiometer_loop() {

    ed_potentiometer_value_raw = analogRead(A0);

    ed_potentiometer_value_voltage = map(ed_potentiometer_value_raw, 
                                        ED_POTENTIOMETER_RAW_MIN, 
                                        ED_POTENTIOMETER_RAW_MAX, 
                                        ED_POTENTIOMETER_VOLTAGE_MIN, 
                                        ED_POTENTIOMETER_VOLTAGE_MAX);
}

int ed_potentiometer_get_raw() {

    return ed_potentiometer_value_raw;
}

int ed_potentiometer_get_voltage() {
    
    return ed_potentiometer_value_voltage;
}