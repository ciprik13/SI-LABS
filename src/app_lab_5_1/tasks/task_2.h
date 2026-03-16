#ifndef APP_LAB_5_1_TASK_2_H
#define APP_LAB_5_1_TASK_2_H

#include "task_config.h"

// Task 2 – Signal Conditioning  (50 ms, priority 2)
//
// Reads g51_cmd (from task_1) and applies:
//   1. Saturation  – clamp raw_cmd to valid binary set {0, 1}
//   2. Debounce    – require BIN_DEBOUNCE_SAMPLES consecutive identical
//                    samples before committing a state change
//
// Writes g51_bin.committed (protected by g51_bin_mutex).
// task51_binary_ctrl then reads committed to drive the pin.
//
// Owns g51_cmd, g51_bin and their mutexes (defined in task_2.cpp).
// Call task51_init() before starting the scheduler.

void task51_signal_cond(void *pvParameters);

#endif // APP_LAB_5_1_TASK_2_H