#ifndef APP_LAB_4_2_TASK_3_H
#define APP_LAB_4_2_TASK_3_H

#include <Arduino_FreeRTOS.h>

// Task 3 – Display & Reporting
//
// Period  : 500 ms
// Priority: 1 (lowest)
//
// LCD (16×2):
//   Row 0: S1 raw → conditioned value + alert status
//   Row 1: S2 raw → conditioned value + alert status
//
// Serial: full structured report with all four pipeline stages per sensor
//   (raw, saturated, median, weighted avg) plus thresholds and debounce state.
void task42_report(void *pvParameters);

#endif // APP_LAB_4_2_TASK_3_H
