#ifndef TASK_3_72_H
#define TASK_3_72_H

// ===========================================================================
// task_3 – LCD + Serial display reporter  (Task 3)
//
// Period  : REPORT_PERIOD_MS = 500 ms
// Priority: 2
//
// LCD 16x2 layout (normal mode):
//   Row 0: "EV:GRN  NS:RED  "   (3-char colour code per direction)
//   Row 1: "T:XXXXms [REQ]  "   (ms in current state, request flag)
//
// LCD 16x2 layout (night mode):
//   Row 0: "** NIGHT MODE **"
//   Row 1: "  Yellow blink  "
//
// Serial block (on change or heartbeat):
//   [T=Xms] Lab 7.2 Traffic =======
//    STATE  : EV_GREEN  (T: 2400ms)
//    EV     : GRN | NS : RED
//    REQUEST: YES
//    NIGHT  : OFF
//   ================================
//
// #define SERIAL_MODE_PLOTTER 0  (0=monitor, 1=plotter)
// ===========================================================================

#define SERIAL_MODE_PLOTTER 0

void task3_72_run(void *pvParameters);

#endif