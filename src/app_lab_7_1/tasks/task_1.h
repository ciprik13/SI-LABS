#ifndef TASK_1_71_H
#define TASK_1_71_H

#include "task_config.h"

// ===========================================================================
// task_1 – Serial command decoder  (Task 1)
//
// Period  : CMD_PERIOD_MS = 10 ms
// Priority: 1 (lowest – display / control take precedence)
//
// Non-blocking: scanf("%c") called ONLY after Serial.available() check,
// so srv_serial_get_char() returns immediately without blocking.
// Characters accumulate in rx_line; sscanf dispatches on newline.
//
// Accepted commands (case-insensitive):
//   STATUS  – force immediate serial report from task_3
//   HELP    – print command list
//
// Lifecycle:
//   task1_71_setup()    – call once before vTaskStartScheduler
//   task1_71_get_cmd()  – thread-safe snapshot of App71UserCmd_t
//   task1_71_run()      – FreeRTOS task entry point
// ===========================================================================

void           task1_71_setup();
App71UserCmd_t task1_71_get_cmd();
void           task1_71_run(void *pvParameters);

#endif // TASK_1_71_H
