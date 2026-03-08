#include "dd_sns_dht.h"
#include "ed_dht/ed_dht.h"
#include <Arduino_FreeRTOS.h>
#include <semphr.h>

static SemaphoreHandle_t s_mutex    = NULL;
static int               s_raw      = 0;
static int               s_celsius  = 0;
static int               s_humidity = 0;

void dd_sns_dht_setup() {
    s_mutex = xSemaphoreCreateMutex();
    ed_dht_setup();
}

void dd_sns_dht_loop() {
    ed_dht_loop();   // throttled internally – safe to call every 50 ms

    int raw      = ed_dht_get_raw();
    int celsius  = ed_dht_get_celsius();
    int humidity = ed_dht_get_humidity();

    if (xSemaphoreTake(s_mutex, portMAX_DELAY) == pdTRUE) {
        s_raw      = raw;
        s_celsius  = celsius;
        s_humidity = humidity;
        xSemaphoreGive(s_mutex);
    }
}

int dd_sns_dht_get_raw() {
    int v = 0;
    if (xSemaphoreTake(s_mutex, portMAX_DELAY) == pdTRUE) {
        v = s_raw;
        xSemaphoreGive(s_mutex);
    }
    return v;
}

int dd_sns_dht_get_celsius() {
    int v = 0;
    if (xSemaphoreTake(s_mutex, portMAX_DELAY) == pdTRUE) {
        v = s_celsius;
        xSemaphoreGive(s_mutex);
    }
    return v;
}

int dd_sns_dht_get_humidity() {
    int v = 0;
    if (xSemaphoreTake(s_mutex, portMAX_DELAY) == pdTRUE) {
        v = s_humidity;
        xSemaphoreGive(s_mutex);
    }
    return v;
}
