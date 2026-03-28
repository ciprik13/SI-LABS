#include "app_lab_5_2.h"
#include "tasks/task_1.h"
#include "tasks/task_2.h"
#include "tasks/task_3.h"
#include "tasks/task_config.h"
#include "act_binary/act_binary.h"
#include "act_analog/act_analog.h"
#include "srv_serial_stdio/srv_serial_stdio.h"
#include "srv_stdio_lcd/srv_stdio_lcd.h"
#include "dd_led/dd_led.h"
#include <Arduino.h>
#include <Arduino_FreeRTOS.h>

// ===========================================================================
// app_lab_5_2 – Dual-actuator control  (Lab 5.2, Variant C)
//
// Hardware:
//   Binary actuator : relay on pin 13
//                     Relay NO contact → load LED (turns on with relay)
//   Analog actuator : DC motor via L298N
//                       ENA → pin 10 (PWM speed)
//                       IN1 → pin 8
//                       IN2 → pin 7
//   Potentiometer   : A0  (used in AUTO mode)
//   Status LEDs     : RED=9 (relay mirror), GREEN=12 (OK), YELLOW=11 (alert)
//   LCD I2C         : 16×2 at 0x27
//
// FreeRTOS tasks:
//   task1_run  – priority 3, period  20 ms  – Serial command input/decode
//   task2_run  – priority 2, period  25 ms  – conditioning + drive
//   task3_run  – priority 1, period 500 ms  – LCD + Serial report
// ===========================================================================

void app_lab_5_2_setup() {
    srv_serial_stdio_setup();
    srv_stdio_lcd_setup();

    // Binary actuator – relay (+ load LED via NO contact)
    act_binary_init(PIN_RELAY);

    // Analog actuator – L298N
    act_analog_init(PIN_MOTOR_ENA, PIN_MOTOR_IN1, PIN_MOTOR_IN2);

    // Status LEDs
    dd_led_setup_with_pins(PIN_LED_BIN_ON, PIN_LED_OK, PIN_LED_ALERT);
    dd_led_turn_off();    // RED   – off
    dd_led_1_turn_on();   // GREEN – OK
    dd_led_2_turn_off();  // YELLOW – no alert

    // Task internals
    task1_setup();
    task2_setup();

    // FreeRTOS tasks
    xTaskCreate(task1_run, "Cmd52",     512, NULL, 3, NULL);
    xTaskCreate(task2_run, "Cond52",    512, NULL, 2, NULL);
    xTaskCreate(task3_run, "Display52", 768, NULL, 1, NULL);
}

void app_lab_5_2_loop() {}
