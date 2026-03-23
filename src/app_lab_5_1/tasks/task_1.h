#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

#include "task_config.h"

// ===========================================================================
// input_handler – Serial line receiver and command decoder  (Task 1)
//
// Period  : ACTUATOR_CMD_PERIOD_MS = 20 ms
// Priority: 3
//
// Each tick drains available Serial bytes into an internal line buffer.
// When a newline is detected the accumulated line is decoded and the
// internal actuator intent registers are updated atomically.
//
// Accepted input (case-insensitive, leading/trailing whitespace ignored):
//   ON            – engage binary actuator
//   OFF           – disengage binary actuator
//   AUTO          – analog level tracks potentiometer
//   PWM <0..255>  – analog level fixed to given value
//   HELP          – print usage reminder
//
// Thread-safe read:
//   input_handler_get_cmd()  – returns assembled App5UserCmd_t snapshot
//
// Lifecycle:
//   input_handler_setup()    – call once before vTaskStartScheduler
//   input_handler_run()      – FreeRTOS task entry point
// ===========================================================================

void          input_handler_setup();
App5UserCmd_t input_handler_get_cmd();
void          input_handler_run(void *pvParameters);

#endif // INPUT_HANDLER_H