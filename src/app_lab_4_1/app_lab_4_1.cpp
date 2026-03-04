#include "app_lab_4_1.h"
#include "tasks/task_1.h"
#include "tasks/task_2.h"
#include "tasks/task_3.h"
#include "srv_serial_stdio/srv_serial_stdio.h"
#include "srv_stdio_lcd/srv_stdio_lcd.h"
#include "dd_sns_temperature/dd_sns_temperature.h"
#include "dd_led/dd_led.h"
#include <Arduino_FreeRTOS.h>

// ===========================================================================
// Application entry point – Lab 4.1
//
// Module stack:
//   ed_potentiometer       – raw ADC driver (A0), potentiometer as temp sensor
//   dd_sns_temperature     – conversion to voltage + Celsius, sensor mutex
//   task_1 (Acquisition)   – 50 ms, priority 3
//   task_2 (Conditioning)  – 50 ms (+10 ms offset), priority 2
//                            hysteresis + antibounce + LED visual indicator
//   task_3 (Report)        – 500 ms, priority 1
//                            structured printf via LCD (STDIO)
// ===========================================================================

void app_lab_4_1_setup() {
    srv_stdio_lcd_setup();            // init LCD (stream available, stdout untouched)
    srv_serial_stdio_setup();         // route stdout -> Serial (terminal visible)
    dd_sns_temperature_setup();       // init temperature sensor + sensor mutex
    dd_led_setup();                   // RED=pin13  GREEN=pin12  YELLOW=pin11

    task_2_init();                    // create g_cond_mutex (owned by task_2)

    // Priority: acquisition (3) > conditioning (2) > report (1)
    xTaskCreate(task_acquisition,  "Acquisition",  256, NULL, 3, NULL);
    xTaskCreate(task_conditioning, "Conditioning", 256, NULL, 2, NULL);
    xTaskCreate(task_report,       "Report",       512, NULL, 1, NULL);
}

void app_lab_4_1_loop() {
    // FreeRTOS scheduler takes over; loop intentionally empty.
}