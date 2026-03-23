#ifndef ACTUATOR_CTRL_H
#define ACTUATOR_CTRL_H

#include "task_config.h"

// ===========================================================================
// actuator_ctrl – Signal conditioning and actuator drive loop  (Task 2)
//
// Period  : ACTUATOR_COND_PERIOD_MS = 25 ms
// Priority: 2
//
// Each cycle the task:
//   - samples the potentiometer
//   - fetches the current user intent from input_handler
//   - drives the binary actuator through its debounce layer
//   - selects a PWM target (pot-mapped or fixed) and drives the motor
//   - updates the over-speed alert using two-threshold hysteresis
//   - publishes a complete system snapshot for the display task
//   - reflects binary and alert states on the three status LEDs
//
// The alert state is maintained as persistent module-level state so it
// does NOT need to be read back from the shared snapshot each cycle.
//
// Owns g_app5_snapshot and g_app5_snapshot_mutex (defined in task_2.cpp).
//
// Lifecycle:
//   actuator_ctrl_setup()  – call once before vTaskStartScheduler
//   actuator_ctrl_run()    – FreeRTOS task entry point
// ===========================================================================

void actuator_ctrl_setup();
void actuator_ctrl_run(void *pvParameters);

#endif // ACTUATOR_CTRL_H