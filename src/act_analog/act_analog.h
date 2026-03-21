#ifndef ACT_ANALOG_H
#define ACT_ANALOG_H

#include <stdbool.h>
#include <Arduino.h>

// ===========================================================================
// act_analog – Analog Actuator Module (L298N motor driver / PWM)
//
// Public API – all internal state is hidden in act_analog.cpp.
// Supports L298N: init() takes ENA, IN1, IN2 pins.
// When PWM > 0: IN1=HIGH, IN2=LOW (forward).
// When PWM = 0: IN1=LOW,  IN2=LOW (coast/stop).
// ===========================================================================

// Call once before the FreeRTOS scheduler starts.
void act_analog_init(uint8_t ena_pin, uint8_t in1_pin, uint8_t in2_pin);

// Run one saturation + debounce + hardware-write cycle.
// Pass raw PWM value (0-255) directly.
// Call from the conditioning task at a fixed period.
void act_analog_tick(int raw_pwm);

// Returns current applied PWM value (0-255). Thread-safe.
int  act_analog_get_pwm();

// Returns true if the over-threshold alert is committed. Thread-safe.
bool act_analog_get_alert();

// Returns alert debounce count. Thread-safe.
int  act_analog_get_bounce_count();

// Returns configuration constants (for display/reporting use)
int  act_analog_get_alert_threshold_high();
int  act_analog_get_alert_threshold_low();
int  act_analog_get_debounce_samples();

#endif // ACT_ANALOG_H