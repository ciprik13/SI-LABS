#ifndef TASK_3_62_H
#define TASK_3_62_H

// ===========================================================================
// task_3 – LCD + Serial display reporter  (Task 3)
//
// Period  : REPORT_PERIOD_MS = 500 ms
// Priority: 1 (lowest)
//
// LCD 16×2 layout:
//
//   Row 0 (16 chars):
//     AUTO:    "T:23.4 SP:22 AU"
//     MANUAL:  "T:23.4 SP:22 MA"
//     Alarm:   "T:23.4 SP:22 AL"
//
//   Row 1 (16 chars):
//     "OUT:75% E:+1.4 "
//
// Serial block (on change or heartbeat):
//   [T=xxxms] Lab 6.2 PID ============
//    TEMP   : 23.4°C  (raw: 234)
//    HUMID  : 55 %RH
//    SETPNT : SP=22°C
//    ERROR  : +1.40°C
//    PID    : P=21.00  I= 8.50  D= 0.25
//    OUTPUT : 75.0 %  [AUTO]
//    RELAY  : [ON ]
//    ALARM  : [OK]
//   =====================================
//
// Arduino Serial Plotter line (every tick):
//   SetPoint:22  Value:23  Output:75
// ===========================================================================

void task3_62_run(void *pvParameters);

#endif // TASK_3_62_H