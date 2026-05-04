#ifndef TASK_2_62_H
#define TASK_2_62_H

#include "task_config.h"

// ===========================================================================
// task_2 – DHT acquisition + PID control loop  (Task 2)
//
// Period  : COND_PERIOD_MS = 100 ms  (10 Hz control tick)
// Priority: 2
//
// Each cycle:
//   1. Read user intent from task1_62_get_cmd()
//   2. Call ed_dht_loop() – throttled to DHT min interval (~1 s)
//   3. Compute PID error: e = SP - temp
//   4. AUTO mode:
//        P = Kp * e
//        I += Ki * e * dt           (anti-windup: clamped to output limits)
//        D  = Kd * (e - e_prev) / dt
//        output = clamp(P + I + D, 0, 100)
//   5. MANUAL mode:
//        output = manual_duty (from DUTY command)
//        Integrator frozen; pre-loaded on return to AUTO (bumpless transfer)
//   6. Time-proportional relay: within RELAY_WINDOW_MS, relay ON for
//        (output/100) * RELAY_WINDOW_MS ms then OFF for the remainder.
//   7. Evaluate alarm: |error| > ALARM_THRESHOLD → RED blink
//   8. Update LEDs
//   9. Publish snapshot under mutex
//
// Lifecycle:
//   task2_62_setup()  – call once before vTaskStartScheduler
//   task2_62_run()    – FreeRTOS task entry point
// ===========================================================================

void task2_62_setup();
void task2_62_run(void *pvParameters);

#endif // TASK_2_62_H