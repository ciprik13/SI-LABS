#ifndef TASK_2_H
#define TASK_2_H

#include "task_config.h"

// Task 2 – Signal Conditioning / Threshold Alerting
// Period : 50 ms  (10 ms offset after task_1)
// Priority: 2
//
// Owns g_cond and g_cond_mutex.
// Call task_2_init() once before starting the FreeRTOS scheduler.
void task_2_init();
void task_conditioning(void *pvParameters);

#endif