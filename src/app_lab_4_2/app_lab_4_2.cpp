#include "app_lab_4_2.h"
#include "tasks/task_1.h"
#include "tasks/task_2.h"
#include "tasks/task_3.h"
#include "srv_serial_stdio/srv_serial_stdio.h"
#include "srv_stdio_lcd/srv_stdio_lcd.h"
#include "dd_sns_temperature/dd_sns_temperature.h"
#include "dd_sns_dht/dd_sns_dht.h"
#include "dd_sns_gas/dd_sns_gas.h"
#include "dd_led/dd_led.h"
#include <Arduino_FreeRTOS.h>

// ===========================================================================
// Application entry point – Lab 4.2  (Variant C – two sensors, full pipeline)
//
// Hardware identical to Lab 4.1 + MQ-2 gas sensor on A1:
//   S1: Potentiometer on A0  → dd_sns_temperature  (analog, 0..100 °C)
//   S2: DHT11/DHT22 on pin 2 → dd_sns_dht          (digital, temp + %RH)
//   S3: MQ-2 gas sensor on A1→ dd_sns_gas          (analog, 0..100 %)
//   LEDs: RED=13  GREEN=12  YELLOW=11
//
// Key difference vs Lab 4.1:
//   Task 2 now runs the full signal conditioning pipeline per sensor:
//     raw → saturation → median filter (window=7) → weighted moving average
//         → hysteresis + debounce → committed alert
//   Task 3 reports all four intermediate values on Serial; LCD shows
//     raw vs conditioned value side-by-side per sensor.
//
// FreeRTOS task schedule:
//   task42_acquisition  – 50 ms,  priority 3  (both sensors)
//   task42_conditioning – 50 ms,  priority 2  (+10 ms offset)
//   task42_report       – 500 ms, priority 1
// ===========================================================================

void app_lab_4_2_setup() {
    srv_serial_stdio_setup();         // init Serial (baud rate, etc.)
    srv_stdio_lcd_setup();            // stdout → LCD stream (tees to Serial)
    dd_sns_temperature_setup();       // S1: analog potentiometer + mutex
    dd_sns_dht_setup();               // S2: digital DHT11/DHT22 (pin 2) + mutex
    dd_sns_gas_setup();               // S3: analog gas sensor MQ-2 (pin A1) + mutex
    dd_led_setup();                   // RED=13  GREEN=12  YELLOW=11

    task42_init();                    // create g42_cond1_mutex + g42_cond2_mutex

    // Priority order: acquisition (3) > conditioning (2) > report (1)
    xTaskCreate(task42_acquisition,  "Acq42",  512, NULL, 3, NULL);
    xTaskCreate(task42_conditioning, "Cond42", 512, NULL, 2, NULL);
    xTaskCreate(task42_report,       "Rpt42",  768, NULL, 1, NULL);
}

void app_lab_4_2_loop() {
    // FreeRTOS scheduler takes over; loop intentionally empty.
}
