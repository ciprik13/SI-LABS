#include "app_lab_6_2.h"
#include "tasks/task_1.h"
#include "tasks/task_2.h"
#include "tasks/task_3.h"
#include "tasks/task_config.h"
#include "srv_serial_stdio/srv_serial_stdio.h"
#include "srv_stdio_lcd/srv_stdio_lcd.h"
#include "dd_led/dd_led.h"
#include <Arduino.h>
#include <Arduino_FreeRTOS.h>

SemaphoreHandle_t g_app62_io_mutex = NULL;

// ===========================================================================
// app_lab_6_2 – PID Temperature Control  (Lab 6.2, Variant A)
//
// Hardware:
//   Sensor   : DHT22 on pin 2  (ed_dht driver)
//   Actuator : Relay on pin 6  (heater – time-proportional, driven by PID)
//   Status LEDs:
//     RED    (pin  9) – blinks 250 ms on alarm (|error| > ALARM_THRESHOLD)
//     GREEN  (pin 12) – solid ON when output = 0 (comfortable)
//     YELLOW (pin 11) – solid ON when output > 0 (heater active)
//   LCD I2C  : 16×2 at 0x27
//
// FreeRTOS tasks:
//   task1_62_run  – priority 3, period  20 ms  – Serial command decode
//   task2_62_run  – priority 2, period 100 ms  – PID control tick
//   task3_62_run  – priority 1, period 500 ms  – LCD + Serial + Plotter
//
// Bonus:
//   Manual/Auto mode switch via MODE AUTO / MODE MANUAL + DUTY commands.
//   Bumpless transfer: integrator pre-loaded on return to AUTO.
// ===========================================================================

void app_lab_6_2_setup() {
    srv_serial_stdio_setup();
    g_app62_io_mutex = xSemaphoreCreateMutex();

    // Relay pin – start OFF
    pinMode(PIN_RELAY, OUTPUT);
    digitalWrite(PIN_RELAY, LOW);

    // Status LEDs: RED=9, GREEN=12, YELLOW=11
    dd_led_setup_with_pins(PIN_LED_RED, PIN_LED_GREEN, PIN_LED_YELLOW);
    dd_led_turn_off();    // RED    – off
    dd_led_1_turn_on();   // GREEN  – comfortable at startup
    dd_led_2_turn_off();  // YELLOW – heater off

    // Initialise LCD before scheduler starts so Wire/TWI is ready.
    // This mirrors how srv_stdio_lcd_setup() works in other labs.
    lcd.init();
    lcd.backlight();
    lcd.clear();

    // Task internals
    task1_62_setup();
    task2_62_setup();

    // FreeRTOS tasks
    xTaskCreate(task1_62_run, "Cmd62",  512, NULL, 3, NULL);
    xTaskCreate(task2_62_run, "Ctrl62", 512, NULL, 2, NULL);
    xTaskCreate(task3_62_run, "Disp62", 768, NULL, 1, NULL);
}

void app_lab_6_2_loop() {}