#ifndef APP_LAB_5_1_TASK_1_H
#define APP_LAB_5_1_TASK_1_H

// Task 1 – Command Input
//
// Period  : 50 ms (vTaskDelayUntil)
// Priority: 3 (highest)
//
// Reads binary actuator commands from Serial STDIO (non-blocking).
// Accepted commands (case-insensitive): 1 / ON / 0 / OFF / STATUS
//
// Invalid input sets g51_cmd.invalid_cmd = true so the display task
// can show a warning message for one cycle.
//
// Updates g51_cmd (protected by g51_cmd_mutex).

void task51_cmd_input(void *pvParameters);

#endif // APP_LAB_5_1_TASK_1_H