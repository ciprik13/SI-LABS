#ifndef TASK_1_61_H
#define TASK_1_61_H

#include "task_config.h"

// ===========================================================================
// task_1 – Serial command decoder  (Task 1)
//
// Period  : CMD_PERIOD_MS = 20 ms
// Priority: 3
//
// Maintains setpoint and hysteresis intent registers (mutex-protected).
// Non-blocking character-by-character input via srv_serial_stdio_try_get_char().
//
// Accepted commands (case-insensitive):
//   SP <value>   – set integer setpoint (SP_MIN..SP_MAX)
//   SP+          – increment setpoint by SP_STEP
//   SP-          – decrement setpoint by SP_STEP
//   HYST <value> – set hysteresis band (HYST_MIN..HYST_MAX)
//   STATUS       – force an immediate serial report from task_3
//   HELP         – print command list
//
// Lifecycle:
//   task1_61_setup()    – call once before vTaskStartScheduler
//   task1_61_get_cmd()  – thread-safe snapshot of App61UserCmd_t
//   task1_61_run()      – FreeRTOS task entry point
// ===========================================================================

void           task1_61_setup();
App61UserCmd_t task1_61_get_cmd();
void           task1_61_run(void *pvParameters);

#endif // TASK_1_61_H