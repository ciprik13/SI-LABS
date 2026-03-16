#include "dd_sns_angle.h"
#include "ed_potentiometer/ed_potentiometer.h"
#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <Arduino.h>

// ---- Internal shared state (written by acquisition task, read by others) ----
static SemaphoreHandle_t s_mutex   = NULL;
static int               s_raw     = 0;
static int               s_voltage = 0;
static int               s_angle   = 0;   // centred: -135..+135 degrees

// -----------------------------------------------------------------------------

void dd_sns_angle_setup() {
    s_mutex = xSemaphoreCreateMutex();
    ed_potentiometer_setup();
}

// Called exclusively from the acquisition task – no mutex needed for the write
// because only one task ever calls this function.
void dd_sns_angle_loop() {
    ed_potentiometer_loop();

    int raw     = ed_potentiometer_get_raw();
    int voltage = ed_potentiometer_get_voltage();
    int angle   = (int)map((long)voltage,
                           ED_POTENTIOMETER_VOLTAGE_MIN,
                           ED_POTENTIOMETER_VOLTAGE_MAX,
                           DD_SNS_ANGLE_MIN,
                           DD_SNS_ANGLE_MAX) - DD_SNS_ANGLE_CENTER;

    if (xSemaphoreTake(s_mutex, portMAX_DELAY) == pdTRUE) {
        s_raw     = raw;
        s_voltage = voltage;
        s_angle   = angle;
        xSemaphoreGive(s_mutex);
    }
}

int dd_sns_angle_get_raw() {
    int v = 0;
    if (xSemaphoreTake(s_mutex, portMAX_DELAY) == pdTRUE) {
        v = s_raw;
        xSemaphoreGive(s_mutex);
    }
    return v;
}

int dd_sns_angle_get_voltage() {
    int v = 0;
    if (xSemaphoreTake(s_mutex, portMAX_DELAY) == pdTRUE) {
        v = s_voltage;
        xSemaphoreGive(s_mutex);
    }
    return v;
}

int dd_sns_angle_get_value() {
    int v = 0;
    if (xSemaphoreTake(s_mutex, portMAX_DELAY) == pdTRUE) {
        v = s_angle;
        xSemaphoreGive(s_mutex);
    }
    return v;
}

