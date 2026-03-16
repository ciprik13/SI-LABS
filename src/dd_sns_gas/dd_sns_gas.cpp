#include "dd_sns_gas.h"
#include "ed_gas/ed_gas.h"
#include <Arduino_FreeRTOS.h>
#include <semphr.h>

static SemaphoreHandle_t s_mutex   = NULL;
static int               s_raw     = 0;
static int               s_voltage = 0;
static int               s_percent = 0;

void dd_sns_gas_setup() {
    s_mutex = xSemaphoreCreateMutex();
    ed_gas_setup();
}

void dd_sns_gas_loop() {
    ed_gas_loop();

    int raw     = ed_gas_get_raw();
    int voltage = ed_gas_get_voltage();
    int percent = ed_gas_get_percent();

    if (xSemaphoreTake(s_mutex, portMAX_DELAY) == pdTRUE) {
        s_raw     = raw;
        s_voltage = voltage;
        s_percent = percent;
        xSemaphoreGive(s_mutex);
    }
}

int dd_sns_gas_get_raw() {
    int v = 0;
    if (xSemaphoreTake(s_mutex, portMAX_DELAY) == pdTRUE) {
        v = s_raw;
        xSemaphoreGive(s_mutex);
    }
    return v;
}

int dd_sns_gas_get_voltage() {
    int v = 0;
    if (xSemaphoreTake(s_mutex, portMAX_DELAY) == pdTRUE) {
        v = s_voltage;
        xSemaphoreGive(s_mutex);
    }
    return v;
}

int dd_sns_gas_get_percent() {
    int v = 0;
    if (xSemaphoreTake(s_mutex, portMAX_DELAY) == pdTRUE) {
        v = s_percent;
        xSemaphoreGive(s_mutex);
    }
    return v;
}
