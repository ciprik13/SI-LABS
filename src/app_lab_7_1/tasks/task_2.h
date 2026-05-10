#ifndef TASK_2_71_H
#define TASK_2_71_H

#include "task_config.h"

// ===========================================================================
// task_2 – Button acquisition + FSM control  (Task 2)
//
// Period  : COND_PERIOD_MS = 20 ms
// Priority: 3 (highest – time-critical button sampling)
//
// Each cycle:
//   1. Read user intent from task1_71_get_cmd()
//   2. Sample button: digitalRead(PIN_BTN) == LOW → pressed
//   3. Debounce state machine:
//        Accumulate consecutive LOW samples; once DEBOUNCE_SAMPLES met,
//        wait for HIGH (release) to fire a single validated press event.
//        Track how long button is held; flag btn_stuck if > STUCK_THRESHOLD_MS.
//   4. On validated press event: toggle FSM state (LED_OFF ↔ LED_ON)
//      Reset internal debounce counters.
//   5. Drive LEDs:
//        FSM_LED_ON  → RED on, GREEN off
//        FSM_LED_OFF → RED off, GREEN on
//        YELLOW blinks at BLINK_PERIOD_MS when btn_stuck
//   6. Publish snapshot under mutex (vTaskDelayUntil pacing)
//
// Lifecycle:
//   task2_71_setup()  – call once before vTaskStartScheduler
//   task2_71_run()    – FreeRTOS task entry point
// ===========================================================================

void task2_71_setup();
void task2_71_run(void *pvParameters);

#endif // TASK_2_71_H
