#ifndef TASK_1_62_H
#define TASK_1_62_H

#include "task_config.h"

// ===========================================================================
// task_1 – Serial command decoder  (Task 1)
//
// Period  : CMD_PERIOD_MS = 20 ms
// Priority: 3
//
// Non-blocking: scanf("%c") called only after Serial.available() check.
// Characters accumulate in rx_line; sscanf dispatches on newline.
//
// Accepted commands (case-insensitive):
//   SP <val>      – set setpoint (SP_MIN..SP_MAX °C)
//   SP+           – increment setpoint by SP_STEP
//   SP-           – decrement setpoint by SP_STEP
//   KP <val>      – set proportional gain (float)
//   KI <val>      – set integral gain (float)
//   KD <val>      – set derivative gain (float)
//   MODE AUTO     – switch to PID automatic mode (bumpless transfer)
//   MODE MANUAL   – switch to manual override mode
//   DUTY <val>    – set manual duty cycle 0–100 % (MANUAL mode only)
//   STATUS        – force immediate serial report from task_3
//   HELP          – print command list
// ===========================================================================

void           task1_62_setup();
App62UserCmd_t task1_62_get_cmd();
void           task1_62_run(void *pvParameters);

#endif // TASK_1_62_H