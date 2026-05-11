#ifndef TASK_1_72_H
#define TASK_1_72_H

#include "task_config.h"

// ===========================================================================
// task_1 – Button debounce + Serial command decoder  (Task 1)
//
// Period  : CMD_PERIOD_MS = 20 ms
// Priority: 1
//
// Button logic:
//   BTN_NS   (D2): debounced press → set ns_request = true
//   BTN_MODE (D3): debounced press → set toggle_night = true
//
// Serial commands (case-insensitive):
//   STATUS   – force immediate report
//   HELP     – command list
//   NIGHT    – toggle night mode
//   REQUEST  – simulate NS request
// ===========================================================================

void           task1_72_setup();
App72UserCmd_t task1_72_get_cmd();
void           task1_72_run(void *pvParameters);

#endif