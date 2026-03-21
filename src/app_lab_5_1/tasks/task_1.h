#ifndef APP_LAB_5_1_TASK_1_H
#define APP_LAB_5_1_TASK_1_H

#include "task_config.h"

// Task 1 – Command Input
//
// Period  : ACTUATOR_CMD_PERIOD_MS = 20 ms (vTaskDelayUntil)
// Priority: 3 (highest)
//
// Reads Serial line-by-line (non-blocking byte accumulation).
// On complete line calls sscanf() — never blocks on incomplete token.
//
// Accepted commands (case-insensitive):
//   ON              → bin_requested = true
//   OFF             → bin_requested = false
//   AUTO            → analog_mode = AUTO
//   PWM <0..255>    → analog_mode = MANUAL, manual_pwm = value
//   HELP            → prints command list
//   other           → prints "CMD ERR: Unknown command"
//
// Exposes: task51_task1_init(), task51_task1_get_latest()

void task51_task1_init();
App5UserCmd_t task51_task1_get_latest();
void task51_task1(void *pvParameters);

#endif // APP_LAB_5_1_TASK_1_H