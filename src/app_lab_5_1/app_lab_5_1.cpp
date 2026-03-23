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
#include "dd_led/dd_led.h"
#include <Arduino.h>
#include <Arduino_FreeRTOS.h>

void app_lab_5_1_setup() {
    srv_serial_stdio_setup();
    srv_stdio_lcd_setup();
    dd_sns_angle_setup();

    act_binary_init(13);
    act_analog_init(PIN_MOTOR_ENA, PIN_MOTOR_IN1, PIN_MOTOR_IN2);

    dd_led_setup_with_pins(PIN_LED_BIN_ON, PIN_LED_OK, PIN_LED_ALERT);
    dd_led_turn_off();
    dd_led_1_turn_on();
    dd_led_2_turn_off();

    input_handler_setup();
    actuator_ctrl_setup();

    xTaskCreate(input_handler_run,   "CmdInput",  512, NULL, 3, NULL);
    xTaskCreate(actuator_ctrl_run,   "ActCtrl",   512, NULL, 2, NULL);
    xTaskCreate(display_reporter_run,"Display",   768, NULL, 1, NULL);
}

void app_lab_5_1_loop() {}