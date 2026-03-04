#ifndef TASK_1_H
#define TASK_1_H

#include <Arduino_FreeRTOS.h>

// Task 1 – Sensor Acquisition
// Period : 50 ms (vTaskDelayUntil)
// Priority: 3 (highest)
void task_acquisition(void *pvParameters);

#endif