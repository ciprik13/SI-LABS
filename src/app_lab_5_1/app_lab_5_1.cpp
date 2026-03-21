#include "app_lab_5_1.h"
#include "tasks/task_1.h"
#include "tasks/task_2.h"
#include "tasks/task_3.h"
#include "tasks/task_config.h"
#include "act_binary/act_binary.h"
#include "act_analog/act_analog.h"
#include "srv_serial_stdio/srv_serial_stdio.h"
#include "srv_stdio_lcd/srv_stdio_lcd.h"
#include "dd_sns_angle/dd_sns_angle.h"
#include <Arduino.h>
#include <Arduino_FreeRTOS.h>

// ===========================================================================
// Application entry point – Lab 5.1  (Variant C – Binary + Analog Actuator)
//
// Hardware:
//   Binary actuator : Relay on pin 13 (act_binary) – ON/OFF via Serial
//   Analog actuator : L298N motor driver
//                     ENA=10 (PWM), IN1=8, IN2=7
//   Potentiometer   : A0 → dd_sns_angle → AUTO mode analog level
//   LCD 16×2 I2C    : SDA=20, SCL=21, addr=0x3F
//   Status LEDs     : PIN_LED_BIN_ON=9  (RED)    – binary actuator ON
//                     PIN_LED_OK=12     (GREEN)  – no alert
//                     PIN_LED_ALERT=11  (YELLOW) – analog alert >ANALOG_ALERT_HIGH
//
// FreeRTOS tasks:
//   task51_task1       – 20 ms,  priority 3  (Serial command input)
//   task51_conditioning– 25 ms,  priority 2  (actuator control + snapshot)
//   task51_display     – 500 ms, priority 1  (LCD + Serial report)
// ===========================================================================

void app_lab_5_1_setup() {
    srv_serial_stdio_setup();    // Serial 9600 baud + printf redirect
    srv_stdio_lcd_setup();       // LCD 16×2 I2C stdout tee (addr 0x3F)
    dd_sns_angle_setup();        // potentiometer on A0 + mutex

    // Binary actuator — LED on pin 9 (mirrors motor ON state)
    act_binary_init(13);   // pin 13 = relay module IN

    // Analog actuator — L298N motor driver
    act_analog_init(PIN_MOTOR_ENA, PIN_MOTOR_IN1, PIN_MOTOR_IN2);

    // Status LEDs — direct GPIO (not via dd_led driver)
    pinMode(PIN_LED_BIN_ON, OUTPUT);  digitalWrite(PIN_LED_BIN_ON, LOW);
    pinMode(PIN_LED_OK,     OUTPUT);  digitalWrite(PIN_LED_OK,     HIGH);
    pinMode(PIN_LED_ALERT,  OUTPUT);  digitalWrite(PIN_LED_ALERT,  LOW);

    task51_task1_init();   // creates task_1 internal mutex
    task51_init();         // creates g_app5_snapshot_mutex

    xTaskCreate(task51_task1,        "Task1_51", 512, NULL, 3, NULL);
    xTaskCreate(task51_conditioning, "Task2_51", 512, NULL, 2, NULL);
    xTaskCreate(task51_display,      "Task3_51", 768, NULL, 1, NULL);
}

void app_lab_5_1_loop() {
    // FreeRTOS scheduler takes over; loop intentionally empty.
}