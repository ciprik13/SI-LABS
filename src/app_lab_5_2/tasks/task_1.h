#ifndef TASK_1_52_H
#define TASK_1_52_H

#include "task_config.h"

// ===========================================================================
// task_1 – Serial command decoder  (Task 1)
//
// Period  : CMD_PERIOD_MS = 20 ms
// Priority: 3
//
// Maintains four independent intent registers:
//   s_bin_on      – relay state
//   s_mode        – AUTO or MANUAL
//   s_pwm_manual  – fixed PWM value (valid in MANUAL mode)
//   s_at_max/min  – limit advisory flags (bonus)
//
// AUTO mode: task_2 reads the potentiometer directly each cycle.
// MANUAL mode: s_pwm_manual is used as the pipeline input.
//
// INC/DEC only operate in MANUAL mode (ignored in AUTO).
//
// Lifecycle:
//   task1_setup()  – call once before vTaskStartScheduler
//   task1_get_cmd() – thread-safe read of assembled App52UserCmd_t
//   task1_run()    – FreeRTOS task entry point
// ===========================================================================

void           task1_setup();
App52UserCmd_t task1_get_cmd();
void           task1_run(void *pvParameters);

#endif // TASK_1_52_H
