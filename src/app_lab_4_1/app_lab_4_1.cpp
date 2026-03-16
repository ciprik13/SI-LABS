#include "app_lab_4_1.h"
#include "tasks/task_1.h"
#include "tasks/task_2.h"
#include "tasks/task_3.h"
#include "srv_serial_stdio/srv_serial_stdio.h"
#include "srv_stdio_lcd/srv_stdio_lcd.h"
#include "dd_sns_temperature/dd_sns_temperature.h"
#include "dd_sns_dht/dd_sns_dht.h"
#include "dd_led/dd_led.h"
#include <Arduino_FreeRTOS.h>

// ===========================================================================
// Application entry point – Lab 4.1
//
// Module stack:
//   S1: ed_potentiometer       – raw ADC driver (A0)
//       dd_sns_temperature     – voltage + Celsius conversion, mutex
//   S2: ed_dht                 – DHT22 driver (pin 2, throttled to 2 s)
//       dd_sns_dht             – Celsius + humidity, mutex
//   task_1 (Acquisition)   – 50 ms, priority 3  – reads both sensors
//   task_2 (Conditioning)  – 50 ms, priority 2  – hysteresis + antibounce
//                            for both sensors; combined LED indicator
//   task_3 (Report)        – 500 ms, priority 1
//                            LCD row0=S1, row1=S2; Serial=full report
// ===========================================================================

void app_lab_4_1_setup() {
    srv_serial_stdio_setup();         // init Serial (needed before LCD tee uses it)
    srv_stdio_lcd_setup();            // stdout -> LCD stream (tees to Serial)
    dd_sns_temperature_setup();       // S1: analog potentiometer + mutex
    dd_sns_dht_setup();               // S2: digital DHT22 (pin 2) + mutex
    dd_led_setup();                   // RED=pin13  GREEN=pin12  YELLOW=pin11

    task_2_init();                    // create g_cond_mutex + g_cond2_mutex

    // Priority: acquisition (3) > conditioning (2) > report (1)
    xTaskCreate(task_acquisition,  "Acquisition",  512, NULL, 3, NULL);
    xTaskCreate(task_conditioning, "Conditioning", 384, NULL, 2, NULL);
    xTaskCreate(task_report,       "Report",       768, NULL, 1, NULL);
}

void app_lab_4_1_loop() {
    // FreeRTOS scheduler takes over; loop intentionally empty.
}