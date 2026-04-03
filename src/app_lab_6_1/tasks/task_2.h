#ifndef TASK_2_61_H
#define TASK_2_61_H

#include "task_config.h"

// ===========================================================================
// task_2 – DHT acquisition + ON-OFF hysteresis control loop  (Task 2)
//
// Period  : COND_PERIOD_MS = 25 ms
// Priority: 2
//
// Each cycle:
//   1. Fetch user intent from task1_get_cmd()
//   2. Call ed_dht_loop() – internally throttled to DHT's min interval (1–2 s)
//   3. Read latest temp/humidity from ed_dht_get_*()
//   4. Compute ON-OFF thresholds from SP and HYST:
//        thresh_on  = SP - HYST/2   (relay turns ON  below this)
//        thresh_off = SP + HYST/2   (relay turns OFF above this)
//   5. Apply hysteresis state machine (HEATING direction):
//        if temp < thresh_on  → request relay ON
//        if temp > thresh_off → request relay OFF
//        else                 → hold current state (no-switch zone)
//   6. Drive relay via act_binary_request + act_binary_tick
//   7. Evaluate extreme deviation flag: |temp - SP| > 2 × HYST
//   8. Update status LEDs
//   9. Publish snapshot under mutex
//
// Lifecycle:
//   task2_61_setup()  – call once before vTaskStartScheduler
//   task2_61_run()    – FreeRTOS task entry point
// ===========================================================================

void task2_61_setup();
void task2_61_run(void *pvParameters);

#endif // TASK_2_61_H