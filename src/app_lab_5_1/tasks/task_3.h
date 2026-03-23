#ifndef DISPLAY_REPORTER_H
#define DISPLAY_REPORTER_H

// ===========================================================================
// display_reporter – LCD and Serial output task  (Task 3)
//
// Period  : ACTUATOR_REPORT_PERIOD_MS = 500 ms
// Priority: 1 (lowest)
//
// Every tick:
//   - reads the shared snapshot
//   - refreshes both LCD rows unconditionally
//   - prints a Serial report only when content changed or heartbeat elapsed
//
// Change detection uses a 32-bit FNV-1a hash over the snapshot fields
// instead of field-by-field comparison.
//
// LCD layout (16 chars per row):
//   Row 0:  "RLY:ON  MOT:178 "
//   Row 1:  "ALT:OK  ANG:+45d"
//
// Serial block format:
//   [T=<ms>ms] Lab 5.1 ================
//    RELAY  : [ON ] debounce:STB
//    MOTOR  : PWM=178/255 [======    ]
//             mode:AUTO  angle: +45deg
//             req:178  applied:178
//    ALERT  : [OK]         HI=220 LO=200
//   =================================
//
// Entry point:  display_reporter_run()
// ===========================================================================

void display_reporter_run(void *pvParameters);

#endif // DISPLAY_REPORTER_H