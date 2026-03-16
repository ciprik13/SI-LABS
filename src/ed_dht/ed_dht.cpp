#include "ed_dht.h"
#include <DHT.h>
#include <Arduino.h>
#include <Arduino_FreeRTOS.h>

// ===========================================================================
// SENSOR TYPE – schimbă o singură linie în funcție de hardware:
//
//   Wokwi (simulator)  → DHT22
//   Placă fizică        → DHT11
//
// Pasul 1: decomentează linia corectă mai jos.
// Pasul 2: în ed_dht.h ajustează ED_DHT_MIN_INTERVAL_MS:
//            DHT22 → 2000 ms  |  DHT11 → 1000 ms
// ===========================================================================
#define DHTTYPE DHT11      // ← Wokwi / simulator
// #define DHTTYPE DHT11   // ← Placă fizică (DHT11)

static DHT        s_dht(ED_DHT_PIN, DHTTYPE);
static int        s_raw      = 0;
static int        s_celsius  = 0;
static int        s_humidity = 0;
static TickType_t s_last_tick = 0;

// Interval in FreeRTOS ticks (2000 ms)
#define DHT_INTERVAL_TICKS  pdMS_TO_TICKS(ED_DHT_MIN_INTERVAL_MS)

void ed_dht_setup() {
    s_dht.begin();
    // Force a short warmup before first real read
    s_last_tick = xTaskGetTickCount();
}

// Throttled: actual hardware read only once every ED_DHT_MIN_INTERVAL_MS.
void ed_dht_loop() {
    TickType_t now = xTaskGetTickCount();
    if ((now - s_last_tick) < DHT_INTERVAL_TICKS)
        return;
    s_last_tick = now;

    float t = s_dht.readTemperature();
    float h = s_dht.readHumidity();

    if (!isnan(t)) {
        s_raw     = (int)(t * 10.0f);
        s_celsius = (int)t;
    }
    if (!isnan(h)) {
        s_humidity = (int)h;
    }
}

int ed_dht_get_raw()      { return s_raw;      }
int ed_dht_get_celsius()  { return s_celsius;  }
int ed_dht_get_humidity() { return s_humidity; }
