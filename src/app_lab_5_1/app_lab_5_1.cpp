#include "app_lab_5_1.h"
#include "tasks/task_1.h"
#include "tasks/task_2.h"
#include "tasks/task_3.h"
#include "tasks/task_4.h"
#include "tasks/task_5.h"
#include "srv_serial_stdio/srv_serial_stdio.h"
#include "srv_stdio_lcd/srv_stdio_lcd.h"
#include "dd_led/dd_led.h"
#include "dd_button/dd_button.h"
#include "dd_sns_angle/dd_sns_angle.h"
#include <Arduino_FreeRTOS.h>

// ===========================================================================
// Application entry point – Lab 5.1 (Variant C – Binary + Analog Actuator)
//
// Hardware:
//   Binary actuator : LED on pin 13 (RED) – simulates relay/lamp
//   Analog actuator : PWM output on pin 9  – simulates dimmer (0-255)
//   Input (binary)  : Serial commands "1"/"0"/"ON"/"OFF"/"STATUS"
//                     Keypad keys    '1' (ON) / '0' (OFF)
//   Feedback sensor : Potentiometer on A0 → dd_sns_angle (0..270°)
//                     mapped to PWM level 0–100 % for analog actuator
//   Display         : LCD 16×2 (I2C) + Serial
//   Status LEDs     : GREEN (pin 12) = binary actuator ON
//                     YELLOW (pin 11) = analog actuator active (>10 %)
//
// FreeRTOS task schedule:
//   task51_cmd_input      – 50 ms,  priority 3  (Serial + keypad reader)
//   task51_signal_cond    – 50 ms,  priority 2  (debounce + saturation)
//   task51_binary_ctrl    – 75 ms,  priority 2  (binary actuator driver)
//   task51_analog_ctrl    – 75 ms,  priority 2  (analog actuator driver)
//   task51_display        – 500 ms, priority 1  (LCD + Serial report)
//
// Variant C justification:
//   - Binary actuator  : LED (relay-equivalent) – ON/OFF commanded by user
//   - Analog actuator  : PWM dimmer – level follows potentiometer position
//   - Both actuators displayed on LCD with alerts
// ===========================================================================

void app_lab_5_1_setup() {
    srv_serial_stdio_setup();     // Serial (9600 baud) + printf/scanf redirect
    srv_stdio_lcd_setup();        // LCD 16×2 I2C + tee to Serial
    dd_led_setup();               // RED=13, GREEN=12, YELLOW=11
    dd_button_setup();            // physical buttons (optional, same pins as keypad)
    dd_sns_angle_setup();         // potentiometer on A0 → analog actuator level

    task51_init();                // create all shared mutexes / semaphores

    // Higher number = higher FreeRTOS priority
    xTaskCreate(task51_cmd_input,   "Cmd51",   384, NULL, 3, NULL);
    xTaskCreate(task51_signal_cond, "Cond51",  256, NULL, 2, NULL);
    xTaskCreate(task51_binary_ctrl, "Bin51",   256, NULL, 2, NULL);
    xTaskCreate(task51_analog_ctrl, "Anlg51",  256, NULL, 2, NULL);
    xTaskCreate(task51_display,     "Disp51",  512, NULL, 1, NULL);
}

void app_lab_5_1_loop() {
    // FreeRTOS scheduler takes over; loop intentionally empty.
}