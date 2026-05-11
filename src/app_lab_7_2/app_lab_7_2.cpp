#include "app_lab_7_2.h"
#include "tasks/task_1.h"
#include "tasks/task_2.h"
#include "tasks/task_3.h"
#include "tasks/task_config.h"
#include "srv_serial_stdio/srv_serial_stdio.h"
#include "srv_stdio_lcd/srv_stdio_lcd.h"
#include <Arduino.h>
#include <Arduino_FreeRTOS.h>

SemaphoreHandle_t g_app72_io_mutex = NULL;

// ===========================================================================
// app_lab_7_2 – Smart Traffic Light FSM  (Lab 7.2)
//
// Hardware:
//   Buttons       : NS=2, NIGHT=3, INPUT_PULLUP
//   EV LEDs       : GREEN=12, YELLOW=11, RED=9
//   NS LEDs       : GREEN=8, YELLOW=7, RED=6
//   LCD I2C 16x2  : 0x27 (SDA=D20, SCL=D21)
//
// FreeRTOS tasks:
//   task1_72_run  – priority 1, 20 ms   – button debounce + CMD parser
//   task2_72_run  – priority 3, 100 ms  – FSM tick + LED drive
//   task3_72_run  – priority 2, 500 ms  – LCD + serial report
// ===========================================================================

void app_lab_7_2_setup() {
    srv_serial_stdio_setup();
    g_app72_io_mutex = xSemaphoreCreateMutex();

    // Buttons
    pinMode(PIN_BTN_NS, INPUT_PULLUP);
    pinMode(PIN_BTN_MODE, INPUT_PULLUP);

    // EV LEDs
    pinMode(PIN_EV_GREEN,  OUTPUT);
    pinMode(PIN_EV_YELLOW, OUTPUT);
    pinMode(PIN_EV_RED,    OUTPUT);

    // NS LEDs
    pinMode(PIN_NS_GREEN,  OUTPUT);
    pinMode(PIN_NS_YELLOW, OUTPUT);
    pinMode(PIN_NS_RED,    OUTPUT);

    // Safe initial state: both RED
    digitalWrite(PIN_EV_GREEN,  LOW);
    digitalWrite(PIN_EV_YELLOW, LOW);
    digitalWrite(PIN_EV_RED,    HIGH);
    digitalWrite(PIN_NS_GREEN,  LOW);
    digitalWrite(PIN_NS_YELLOW, LOW);
    digitalWrite(PIN_NS_RED,    HIGH);

    // LCD
    lcd.init();
    lcd.backlight();
    lcd.clear();

    task1_72_setup();
    task2_72_setup();

    xTaskCreate(task1_72_run, "Cmd72",  256, NULL, 1, NULL);
    xTaskCreate(task2_72_run, "FSM72",  384, NULL, 3, NULL);
    xTaskCreate(task3_72_run, "Disp72", 512, NULL, 2, NULL);
}

void app_lab_7_2_loop() {}