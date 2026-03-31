#ifndef TASK_3_52_H
#define TASK_3_52_H

// ===========================================================================
// task_3 – LCD and Serial display reporter  (Task 3)
//
// Period  : REPORT_PERIOD_MS = 500 ms
// Priority: 1 (lowest)
//
// LCD 16x2 layout:
//
//   Row 0 (16 chars):
//     Normal:   "RLY:ON  PWM: 178"
//     Alert:    "RLY:ON  **ALERT*"
//     Limit max:"RLY:OFF PWM: MAX"
//     Limit min:"RLY:OFF PWM:STOP"
//
//   Row 1 (16 chars):
//     AUTO mode:   "AUTO POT:512    "
//     MANUAL mode: "MAN  [========] "
//
// Serial block:
//   [T=12340ms] Lab 5.2 ================
//    RELAY  : [ON ] debounce:STB
//    MODE   : AUTO    pot:512 (map:178)
//    MOTOR  : raw:178 sat:178 med:176 wgt:177
//             ramp:170  applied:170/255
//             [======    ]
//    ALERT  : [OK]  HI=220 LO=200
//    LIMIT  : max:NO  min:NO
//   ====================================
//
// Entry point: task3_run()
// ===========================================================================

void task3_run(void *pvParameters);

#endif // TASK_3_52_H
