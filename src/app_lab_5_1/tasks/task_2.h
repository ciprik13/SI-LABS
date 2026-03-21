#ifndef APP_LAB_5_1_TASK_2_H
#define APP_LAB_5_1_TASK_2_H

#include "task_config.h"

// Task 2 – Signal Conditioning + Actuator Control
//
// Period  : ACTUATOR_COND_PERIOD_MS = 25 ms (vTaskDelayUntil)
// Priority: 2
//
// Each cycle:
//   1. Reads potentiometer via dd_sns_angle_loop()
//   2. Reads latest command from task51_task1_get_latest()
//   3. Applies binary actuator (debounce via act_binary)
//   4. Applies analog actuator (AUTO=pot level, MANUAL=PWM command)
//   5. Computes analog alert with hysteresis
//   6. Writes App5Snapshot_t under g_app5_snapshot_mutex (single lock)
//   7. Updates LED indicators via digitalWrite
//
// Owns g_app5_snapshot, g_app5_snapshot_mutex (defined in task_2.cpp).
// Call task51_init() before starting the scheduler.

void task51_conditioning(void *pvParameters);

#endif // APP_LAB_5_1_TASK_2_H