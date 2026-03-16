#ifndef APP_LAB_5_1_TASK_3_H
#define APP_LAB_5_1_TASK_3_H

// Task 3 – Binary Actuator Control  (75 ms, priority 2)
//
// Reads g51_bin.committed (from task51_signal_cond) and writes the physical
// binary actuator pin (BINARY_ACT_PIN = 13, RED LED / relay).
//
// Also updates the GREEN status LED:
//   GREEN ON  = binary actuator is currently ON
//   GREEN OFF = binary actuator is currently OFF
//
// Exposes actuator_get_state() for query by other tasks.

void task51_binary_ctrl(void *pvParameters);

// Internal interface: returns 1 if binary actuator is ON, 0 if OFF
int binary_actuator_get_state();

#endif // APP_LAB_5_1_TASK_3_H