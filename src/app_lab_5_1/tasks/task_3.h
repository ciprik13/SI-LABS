#ifndef APP_LAB_5_1_TASK_3_H
#define APP_LAB_5_1_TASK_3_H

// Task 3 – Display & Reporting
//
// Period  : ACTUATOR_REPORT_PERIOD_MS = 500 ms (vTaskDelay)
// Priority: 1 (lowest)
//
// LCD 16×2 (updated every tick):
//   Row 0: "B: ON C: ON PD"  (binary state / requested / pending)
//   Row 1: "A: 180 AU AL"    (applied PWM / mode / alert)
//
// Serial: printed only when snapshot changed OR heartbeat (10 s) due.

void task51_display(void *pvParameters);

#endif // APP_LAB_5_1_TASK_3_H