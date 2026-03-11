#ifndef APP_LAB_4_2_TASK_1_H
#define APP_LAB_4_2_TASK_1_H

#include <Arduino_FreeRTOS.h>

// Task 1 – Sensor Acquisition
// Period  : 50 ms (vTaskDelayUntil)
// Priority: 3 (highest)
// Reads both sensors and updates their device-driver internal state.
void task42_acquisition(void *pvParameters);

#endif // APP_LAB_4_2_TASK_1_H
