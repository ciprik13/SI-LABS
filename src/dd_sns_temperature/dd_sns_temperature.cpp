#include "dd_sns_temperature.h"
#include "ed_potentiometer/ed_potentiometer.h"
#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <Arduino.h>

// ---- Internal shared state (written by acquisition task, read by others) ----
static SemaphoreHandle_t s_mutex       = NULL;
static int               s_raw         = 0;
static int               s_voltage     = 0;
static int               s_celsius     = 0;   // 0..100 °C

// -----------------------------------------------------------------------------

void dd_sns_temperature_setup() {
    s_mutex = xSemaphoreCreateMutex();
    ed_potentiometer_setup();
}

// Called exclusively from task_1 (acquisition) – single writer, no race on write.
// Mutex still taken to guarantee visibility on architectures without cache coherency.
void dd_sns_temperature_loop() {
    ed_potentiometer_loop();

    int raw     = ed_potentiometer_get_raw();
    int voltage = ed_potentiometer_get_voltage();

    // Linear mapping: 0–5000 mV → 0–100 °C
    int celsius = (int)map((long)voltage,
                           ED_POTENTIOMETER_VOLTAGE_MIN,
                           ED_POTENTIOMETER_VOLTAGE_MAX,
                           DD_SNS_TEMPERATURE_MIN,
                           DD_SNS_TEMPERATURE_MAX);

    if (xSemaphoreTake(s_mutex, portMAX_DELAY) == pdTRUE) {
        s_raw     = raw;
        s_voltage = voltage;
        s_celsius = celsius;
        xSemaphoreGive(s_mutex);
    }
}

int dd_sns_temperature_get_raw() {
    int v = 0;
    if (xSemaphoreTake(s_mutex, portMAX_DELAY) == pdTRUE) {
        v = s_raw;
        xSemaphoreGive(s_mutex);
    }
    return v;
}

int dd_sns_temperature_get_voltage() {
    int v = 0;
    if (xSemaphoreTake(s_mutex, portMAX_DELAY) == pdTRUE) {
        v = s_voltage;
        xSemaphoreGive(s_mutex);
    }
    return v;
}

int dd_sns_temperature_get_celsius() {
    int v = 0;
    if (xSemaphoreTake(s_mutex, portMAX_DELAY) == pdTRUE) {
        v = s_celsius;
        xSemaphoreGive(s_mutex);
    }
    return v;
}
