#ifndef TASK_2_52_H
#define TASK_2_52_H

#include "task_config.h"

// ===========================================================================
// task_2 – Signal conditioning and actuator drive loop  (Task 2)
//
// Period  : COND_PERIOD_MS = 25 ms
// Priority: 2
//
// Each cycle:
//   1. Fetch user intent from task1_get_cmd()
//   2. Read potentiometer (always, regardless of mode)
//   3. Select pipeline input: pot-mapped value (AUTO) or pwm_manual (MANUAL)
//   4. Run signal conditioning pipeline:
//        input → saturate → median filter → weighted average → ramp
//   5. Write ramped PWM to motor via act_analog_tick()
//   6. Drive relay + relay LED via act_binary module
//   7. Evaluate over-speed alert (two-threshold hysteresis)
//   8. Update status LEDs
//   9. Publish snapshot under mutex
//
// Potentiometer mapping:
//   ADC 0-1023  →  map()  →  PWM 0-255
//   This mapped value enters the conditioning pipeline in AUTO mode.
//
// Relay LED:
//   PIN_RELAY drives both the relay coil and, through the relay's
//   normally-open contact, a load LED.  act_binary controls PIN_RELAY;
//   the LED turns on/off as a consequence of relay state.
//
// Lifecycle:
//   task2_setup()  – call once before vTaskStartScheduler
//   task2_run()    – FreeRTOS task entry point
// ===========================================================================

void task2_setup();
void task2_run(void *pvParameters);

#endif // TASK_2_52_H
