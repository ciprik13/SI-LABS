#ifndef APP_LAB_5_1_TASK_4_H
#define APP_LAB_5_1_TASK_4_H

// Task 4 – Analog Actuator Control  (75 ms, priority 2)
//
// Reads the potentiometer angle via dd_sns_angle_loop() / dd_sns_angle_get_value()
// and maps it to a PWM level (0–100 %) applied to ANALOG_ACT_PIN (pin 9).
//
// Signal conditioning pipeline:
//   raw pot (0..270°) → mapped to 0..100 % → saturation → hysteresis +
//   debounce alert if level > ANLG_ALERT_THRESHOLD %
//
// Updates g51_anlg (protected by g51_anlg_mutex).
// YELLOW LED = analog actuator active (level > 10 %)

void task51_analog_ctrl(void *pvParameters);

#endif // APP_LAB_5_1_TASK_4_H