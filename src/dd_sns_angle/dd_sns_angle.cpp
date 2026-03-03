#include "dd_sns_angle.h"
#include "ed_potentiometer/ed_potentiometer.h"
#include <Arduino.h>

static int s_angle_value = 0;
static const int s_angle_offset = DD_SNS_ANGLE_MAX / 2;

void dd_sns_angle_setup() {
    ed_potentiometer_setup();
}

void dd_sns_angle_loop() {
    ed_potentiometer_loop();
    int voltage = ed_potentiometer_get_voltage();
    s_angle_value = map(voltage,
                        ED_POTENTIOMETER_VOLTAGE_MIN,
                        ED_POTENTIOMETER_VOLTAGE_MAX,
                        DD_SNS_ANGLE_MIN,
                        DD_SNS_ANGLE_MAX) - s_angle_offset;
}

int dd_sns_angle_get_value() {
    return s_angle_value;
}

