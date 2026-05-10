#include "app_lab_7_1.h"
#include "tasks/task_1.h"
#include "tasks/task_2.h"
#include "tasks/task_3.h"
#include "tasks/task_config.h"
#include "srv_serial_stdio/srv_serial_stdio.h"
#include "srv_stdio_lcd/srv_stdio_lcd.h"
#include "dd_led/dd_led.h"
#include "dd_button/dd_button.h"
#include <Arduino.h>
#include <Arduino_FreeRTOS.h>

SemaphoreHandle_t g_app71_io_mutex = NULL;

// ===========================================================================
// app_lab_7_1 – FSM Button-LED Control  (Lab 7.1)
//
// Hardware:
//   Button  : BTN_PIN (pin 2, INPUT_PULLUP) via dd_button driver
//   LED RED (pin  9) – ON in FSM state LED_ON
//   LED GREEN (pin 12) – ON in FSM state LED_OFF
//   LED YELLOW (pin 11) – blinks when button is stuck / held too long
//   LCD I2C  : 16×2 at 0x27
//
// FreeRTOS tasks:
//   task1_71_run  – priority 1, period 10 ms  – Serial command decoder
//   task2_71_run  – priority 3, period 20 ms  – Button sampling + FSM
//   task3_71_run  – priority 2, period 500 ms – LCD + Serial + Plotter
// ===========================================================================

void app_lab_7_1_setup() {
    srv_serial_stdio_setup();
    g_app71_io_mutex = xSemaphoreCreateMutex();

    // Button: INPUT_PULLUP configured inside dd_button_setup()
    dd_button_setup();

    // LEDs: RED=9, GREEN=12, YELLOW=11
    dd_led_setup_with_pins(PIN_LED_RED, PIN_LED_GREEN, PIN_LED_YELLOW);
    dd_led_turn_off();    // RED    – off at startup (FSM_LED_OFF)
    dd_led_1_turn_on();   // GREEN  – on at startup (comfortable/idle)
    dd_led_2_turn_off();  // YELLOW – off at startup

    // Initialise LCD before scheduler starts so Wire/TWI is ready
    lcd.init();
    lcd.backlight();
    lcd.clear();

    // Task internals
    task1_71_setup();
    task2_71_setup();

    // FreeRTOS tasks
    //   task1: CMD decode – priority 1, 10 ms poll
    //   task2: FSM + button acquisition – priority 3, 20 ms (highest, time-critical)
    //   task3: Display reporter – priority 2, 500 ms
    xTaskCreate(task1_71_run, "Cmd71",  256, NULL, 1, NULL);
    xTaskCreate(task2_71_run, "FSM71",  384, NULL, 3, NULL);
    xTaskCreate(task3_71_run, "Disp71", 512, NULL, 2, NULL);
}

void app_lab_7_1_loop() {}
