#ifndef TASK_3_71_H
#define TASK_3_71_H

// ===========================================================================
// task_3 – LCD + Serial display reporter  (Task 3)
//
// Period  : REPORT_PERIOD_MS = 500 ms
// Priority: 2
//
// LCD 16×2 layout:
//
//   Row 0 (16 chars):  "FSM  Lab 7.1    "
//   Row 1 (16 chars):
//     FSM_LED_OFF, no stuck: "LED: OFF        "
//     FSM_LED_ON,  no stuck: "LED: ON  BTN:%lu"  (%lu = press count)
//     btn_stuck active:      "LED: ON  !ALT   "
//
// Serial block (on change or heartbeat):
//   [T=xxxms] Lab 7.1 FSM ============
//    STATE  : LED_ON  (presses: N)
//    BUTTON : RAW=1  DEB=0  STUCK=0
//    LED    : [ON ]
//    ALERT  : [OK]
//   ====================================
//
// Serial Plotter line (every tick):
//   State:1  Button:0
//
// Switch SERIAL_MODE_MONITOR / SERIAL_MODE_PLOTTER via #define below.
// ===========================================================================

// Set to 1 for Serial Plotter, 0 for Monitor block
#define SERIAL_MODE_PLOTTER  0

void task3_71_run(void *pvParameters);

#endif // TASK_3_71_H
