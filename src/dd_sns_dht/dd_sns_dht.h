#ifndef DD_SNS_DHT_H
#define DD_SNS_DHT_H

// Digital temperature/humidity sensor driver (DHT22, pin 2).
// Wraps ed_dht with a FreeRTOS mutex so getters are safe from any task.

void dd_sns_dht_setup();
void dd_sns_dht_loop();          // call from acquisition task (every 50 ms)

// Mutex-protected getters
int  dd_sns_dht_get_raw();       // temperature × 10  (0.1 °C resolution)
int  dd_sns_dht_get_celsius();   // integer °C
int  dd_sns_dht_get_humidity();  // integer %RH

#endif // DD_SNS_DHT_H
