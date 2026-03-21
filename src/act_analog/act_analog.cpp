#include "act_analog.h"
#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <Arduino.h>
#include <stdio.h>

// ===========================================================================
// Analog Actuator – Internal (hidden) state
// Controls L298N: ENA pin (PWM speed), IN1/IN2 (direction).
// ===========================================================================

#define ANLG_ALERT_HIGH        220   // PWM alert ON  edge  (≈86% of 255)
#define ANLG_ALERT_LOW         200   // PWM alert OFF edge (hysteresis)
#define ANLG_DEBOUNCE_SAMPLES    4   // 4 × 25 ms = 100 ms persistence
#define MTX_TIMEOUT              pdMS_TO_TICKS(10)

typedef struct {
    uint8_t ena_pin;
    uint8_t in1_pin;
    uint8_t in2_pin;
    int     pwm_applied;     // 0-255 written to ENA pin
    bool    alert_active;    // committed alert
    bool    pending_alert;   // candidate alert being debounced
    int     bounce_count;    // consecutive confirmations
} AnlgState_t;

static AnlgState_t       s_anlg  = { 0, 0, 0, 0, false, false, 0 };
static SemaphoreHandle_t s_mutex = NULL;

static int clamp(int v, int lo, int hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

// ---------------------------------------------------------------------------
void act_analog_init(uint8_t ena_pin, uint8_t in1_pin, uint8_t in2_pin) {
    s_anlg.ena_pin = ena_pin;
    s_anlg.in1_pin = in1_pin;
    s_anlg.in2_pin = in2_pin;
    s_mutex = xSemaphoreCreateMutex();
    pinMode(ena_pin, OUTPUT);
    pinMode(in1_pin, OUTPUT);
    pinMode(in2_pin, OUTPUT);
    digitalWrite(in1_pin, LOW);
    digitalWrite(in2_pin, LOW);
    analogWrite(ena_pin, 0);
}

// ---------------------------------------------------------------------------
void act_analog_tick(int raw_pwm) {
    int pwm = clamp(raw_pwm, 0, 255);

    if (xSemaphoreTake(s_mutex, MTX_TIMEOUT) != pdTRUE) {
        printf("\r[act_analog] WARNING: mutex timeout in tick()\n");
        return;
    }

    // L298N drive: forward when pwm > 0, coast when pwm == 0
    // analogWrite + digitalWrite inside the lock → hardware always
    // consistent with reported state, no observable window.
    if (pwm > 0) {
        digitalWrite(s_anlg.in1_pin, HIGH);
        digitalWrite(s_anlg.in2_pin, LOW);
    } else {
        digitalWrite(s_anlg.in1_pin, LOW);
        digitalWrite(s_anlg.in2_pin, LOW);
    }
    analogWrite(s_anlg.ena_pin, pwm);
    s_anlg.pwm_applied = pwm;

    // Hysteresis + debounce for alert
    bool desired;
    if      (!s_anlg.alert_active && pwm > ANLG_ALERT_HIGH) desired = true;
    else if ( s_anlg.alert_active && pwm < ANLG_ALERT_LOW)  desired = false;
    else                                                      desired = s_anlg.alert_active;

    if (desired == s_anlg.pending_alert) {
        s_anlg.bounce_count++;
    } else {
        s_anlg.pending_alert = desired;
        s_anlg.bounce_count  = 1;
    }
    if (s_anlg.bounce_count >= ANLG_DEBOUNCE_SAMPLES) {
        s_anlg.alert_active = s_anlg.pending_alert;
        s_anlg.bounce_count = ANLG_DEBOUNCE_SAMPLES;  // cap
    }

    xSemaphoreGive(s_mutex);
}

// ---------------------------------------------------------------------------
int act_analog_get_pwm() {
    int v = 0;
    if (xSemaphoreTake(s_mutex, MTX_TIMEOUT) == pdTRUE) {
        v = s_anlg.pwm_applied;
        xSemaphoreGive(s_mutex);
    }
    return v;
}

bool act_analog_get_alert() {
    bool v = false;
    if (xSemaphoreTake(s_mutex, MTX_TIMEOUT) == pdTRUE) {
        v = s_anlg.alert_active;
        xSemaphoreGive(s_mutex);
    }
    return v;
}

int act_analog_get_bounce_count() {
    int v = 0;
    if (xSemaphoreTake(s_mutex, MTX_TIMEOUT) == pdTRUE) {
        v = s_anlg.bounce_count;
        xSemaphoreGive(s_mutex);
    }
    return v;
}

// Configuration getters — no lock needed (compile-time constants)
int act_analog_get_alert_threshold_high() { return ANLG_ALERT_HIGH; }
int act_analog_get_alert_threshold_low()  { return ANLG_ALERT_LOW;  }
int act_analog_get_debounce_samples()     { return ANLG_DEBOUNCE_SAMPLES; }