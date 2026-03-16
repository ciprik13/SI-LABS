#include "ed_gas.h"
#include <Arduino.h>

static int s_raw     = 0;
static int s_voltage = 0;
static int s_percent = 0;

void ed_gas_setup() {
    // Analog pin is input by default; nothing to configure.
}

void ed_gas_loop() {
    s_raw     = analogRead(ED_GAS_ADC_PIN);

    s_voltage = (int)map((long)s_raw,
                         ED_GAS_RAW_MIN, ED_GAS_RAW_MAX,
                         ED_GAS_VOLTAGE_MIN, ED_GAS_VOLTAGE_MAX);

    s_percent = (int)map((long)s_raw,
                         ED_GAS_RAW_MIN, ED_GAS_RAW_MAX,
                         0, 100);
}

int ed_gas_get_raw()     { return s_raw;     }
int ed_gas_get_voltage() { return s_voltage; }
int ed_gas_get_percent() { return s_percent; }
