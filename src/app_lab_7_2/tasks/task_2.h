#ifndef TASK_2_72_H
#define TASK_2_72_H

#include "task_config.h"

// ===========================================================================
// task_2 – Traffic light FSM + LED drive  (Task 2)
//
// Period  : COND_PERIOD_MS = 100 ms
// Priority: 3 (highest)
//
// Each cycle:
//   1. Fetch cmd from task1_72_get_cmd()
//   2. Apply toggle_night or accumulate ns_request
//   3. Advance FSM timer; check transition conditions
//   4. Drive all 6 LEDs according to current FSM state
//   5. Publish snapshot under mutex (vTaskDelayUntil pacing)
//
// LED drive rules (Moore FSM):
//   EV_GREEN  : EV=GRN, NS=RED
//   EV_YELLOW : EV=YEL, NS=RED
//   ALL_RED_1 : EV=RED, NS=RED
//   NS_GREEN  : EV=RED, NS=GRN
//   NS_YELLOW : EV=RED, NS=YEL
//   ALL_RED_2 : EV=RED, NS=RED
//   NIGHT     : EV=YEL blink, NS=YEL blink (500 ms)
// ===========================================================================

void task2_72_setup();
void task2_72_run(void *pvParameters);

#endif