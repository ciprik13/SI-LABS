#ifndef APP_LAB_5_1_TASK_5_H
#define APP_LAB_5_1_TASK_5_H

// Task 5 – Display & Reporting  (500 ms, priority 1)
//
// LCD 16×2 layout (alternating pages every 500 ms):
//
//   Page 0 – Binary actuator status:
//     Row 0: "RELAY: ON  [DBn/N]"  or "RELAY: OFF [DBn/N]"
//     Row 1: "CMD:ON  SRC:serial"
//
//   Page 1 – Analog actuator status:
//     Row 0: "DIM: XXX% [ALT]"    or "DIM:  XX% [ OK]"
//     Row 1: "PWM:255 Pot:100%"
//
// Serial: full structured report printed every 500 ms.
// Invalid command warning: printed on serial + LCD row 1 for one cycle.

void task51_display(void *pvParameters);

#endif // APP_LAB_5_1_TASK_5_H