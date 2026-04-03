#include "app_lab_6_1.h"
#include "tasks/task_1.h"
#include "tasks/task_2.h"
#include "tasks/task_3.h"
#include "tasks/task_config.h"
#include "act_binary/act_binary.h"
#include "srv_serial_stdio/srv_serial_stdio.h"
#include "srv_stdio_lcd/srv_stdio_lcd.h"
#include "dd_led/dd_led.h"
#include <Arduino.h>
#include <Arduino_FreeRTOS.h>

SemaphoreHandle_t g_app61_io_mutex = NULL;

// ===========================================================================
// app_lab_6_1 – ON-OFF Hysteresis Temperature Control  (Lab 6.1, Variant A)
//
// Hardware:
//   Sensor   : DHT22 on pin 2  (ed_dht driver; change DHTTYPE for DHT11)
//   Actuator : Relay on pin 6  (heater – ON when temp too cold)
//   Status LEDs:
//     RED    (pin  9) – blinks 250 ms on extreme temperature deviation
//     GREEN  (pin 12) – solid ON when comfortable (relay OFF)
//     YELLOW (pin 11) – solid ON when heater active (relay ON)
//   LCD I2C  : 16×2 at 0x27
//
// FreeRTOS tasks:
//   task1_61_run  – priority 3, period  20 ms  – Serial command decode (SP/HYST)
//   task2_61_run  – priority 2, period  25 ms  – DHT acquire + hysteresis control
//   task3_61_run  – priority 1, period 500 ms  – LCD + Serial + Plotter output
// ===========================================================================

void app_lab_6_1_setup() {
    srv_serial_stdio_setup();
    g_app61_io_mutex = xSemaphoreCreateMutex();

    // Relay (heater)
    act_binary_init(PIN_RELAY);

    // Status LEDs: RED=9, GREEN=12, YELLOW=11
    dd_led_setup_with_pins(PIN_LED_RED, PIN_LED_GREEN, PIN_LED_YELLOW);
    dd_led_turn_off();    // RED    – off
    dd_led_1_turn_on();   // GREEN  – comfortable
    dd_led_2_turn_off();  // YELLOW – heater off

    // Task internals
    task1_61_setup();
    task2_61_setup();

    // FreeRTOS tasks
    xTaskCreate(task1_61_run, "Cmd61",     512, NULL, 3, NULL);
    xTaskCreate(task2_61_run, "Ctrl61",    512, NULL, 2, NULL);
    xTaskCreate(task3_61_run, "Disp61",    768, NULL, 1, NULL);
}

void app_lab_6_1_loop() {}