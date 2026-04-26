#ifndef TASK_3_61_H
#define TASK_3_61_H

// ===========================================================================
// task_3 – LCD + Serial display reporter  (Task 3)
//
// Period  : REPORT_PERIOD_MS = 500 ms
// Priority: 1 (lowest)
//
// LCD 16×2 layout:
//
//   Row 0 (16 chars):
//     Normal:       "T:23C SP:22 H:ON"   (relay ON,  no extreme)
//     Extreme dev:  "T:23C SP:22 !!EX"   (extreme deviation)
//     Relay OFF:    "T:22C SP:22 H:OF"
//
//   Row 1 (16 chars):
//     "HU:55%  HY:2 OK "  (comfortable)
//     "HU:55%  HY:2 AL "  (extreme deviation alert)
//
// Serial block (on change or heartbeat):
//   [T=12300ms] Lab 6.1 ================
//    TEMP   : 23°C (raw: 232)
//    HUMID  : 55 %RH
//    SETPNT : SP=22°C  HYST=2°C
//    THRESH : ON<21°C  OFF>23°C
//    RELAY  : [ON ] debounce:STB
//    EXTREME: [OK]
//   ====================================
//
// Arduino Serial Plotter line (every tick, tab-separated labels):
//   SetPoint:22 Value:23 Output:1
//
// Entry point: task3_61_run()
// ===========================================================================

void task3_61_run(void *pvParameters);

#endif // TASK_3_61_H