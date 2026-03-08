#ifndef ED_DHT_H
#define ED_DHT_H

#include <Arduino.h>

// Digital pin connected to DHT22 data line
// For DHT11: same pin, just change DHTTYPE below
#define ED_DHT_PIN  2

// Minimum interval between real sensor reads
// DHT22 spec: 2 s  →  #define ED_DHT_MIN_INTERVAL_MS  2000  (current)
// DHT11 spec: 1 s  →  #define ED_DHT_MIN_INTERVAL_MS  1000  (use this for DHT11)
#define ED_DHT_MIN_INTERVAL_MS  1000

void ed_dht_setup();
void ed_dht_loop();          // call from acquisition task; throttled internally

int  ed_dht_get_raw();       // temperature × 10  (0.1 °C resolution)
int  ed_dht_get_celsius();   // integer °C
int  ed_dht_get_humidity();  // integer %RH

#endif // ED_DHT_H
