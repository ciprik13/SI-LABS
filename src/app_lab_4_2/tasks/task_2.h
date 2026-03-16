#ifndef APP_LAB_4_2_TASK_2_H
#define APP_LAB_4_2_TASK_2_H

#include "task_config.h"

// Task 2 – Signal Conditioning
//
// Full pipeline per sensor each tick:
//   raw → saturation → median filter → weighted moving average
//       → hysteresis + debounce → alert state
//
// Period  : 50 ms  (10 ms offset after task_1)
// Priority: 2
//
// Owns g42_cond1 / g42_cond2 and their mutexes.
// Call task42_init() once before starting the scheduler.
void task42_init();
void task42_conditioning(void *pvParameters);

#endif // APP_LAB_4_2_TASK_2_H
