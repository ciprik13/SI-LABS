#include "task_4.h"
#include "task_config.h"
#include "dd_sns_angle/dd_sns_angle.h"
#include "dd_led/dd_led.h"
#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <Arduino.h>

// ===========================================================================
// Task 4 – Analog Actuator Control  (75 ms period, priority 2)
//
// The potentiometer position (0..270°) is read via dd_sns_angle and mapped
// linearly to a PWM level (0–100 %).  PWM is written to ANALOG_ACT_PIN (9).
//
// Signal conditioning:
//   1. Saturation : clamp to [ANLG_SAT_LOW .. ANLG_SAT_HIGH]
//   2. Hysteresis + debounce : alert if level > ANLG_ALERT_THRESHOLD %
//
// YELLOW LED:
//   ON  = analog actuator active (level > 10 %)
//   OFF = analog actuator at zero or near-zero
// ===========================================================================

// Saturation clamp helper
static int saturate(int val, int lo, int hi) {
    if (val < lo) return lo;
    if (val > hi) return hi;
    return val;
}

void task51_analog_ctrl(void *pvParameters) {
    (void) pvParameters;

    // Initialise PWM pin
    pinMode(ANALOG_ACT_PIN, OUTPUT);
    analogWrite(ANALOG_ACT_PIN, 0);

    // Same offset as task_3 (both are control tasks at same priority)
    vTaskDelay(pdMS_TO_TICKS(20));
    TickType_t xLastWakeTime = xTaskGetTickCount();

    for (;;) {
        // --- Acquire latest potentiometer reading -------------------------
        dd_sns_angle_loop();  // update internal driver state

        // dd_sns_angle_get_value() returns -135..+135°; we want 0..270° total
        // Convert centred angle to 0..270° range, then map to 0..100 %
        int centred    = dd_sns_angle_get_value();         // -135..+135
        int raw_deg    = centred + 135;                    //  0..270
        int raw_pct    = (int)map((long)raw_deg, 0, 270, 0, 100); // 0..100 %

        // --- Signal conditioning: saturation ------------------------------
        int sat_pct = saturate(raw_pct, ANLG_SAT_LOW, ANLG_SAT_HIGH);

        // --- Map to PWM 0-255 ---------------------------------------------
        int pwm_val = (int)map((long)sat_pct, 0, 100, 0, 255);
        analogWrite(ANALOG_ACT_PIN, pwm_val);

        // --- Hysteresis + debounce alert ----------------------------------
        // Read current committed alert state
        bool cur_alert = false;
        if (xSemaphoreTake(g51_anlg_mutex, portMAX_DELAY) == pdTRUE) {
            cur_alert = g51_anlg.alert_active;
            xSemaphoreGive(g51_anlg_mutex);
        }

        bool desired;
        if      (!cur_alert && sat_pct > ANLG_ALERT_THRESHOLD) desired = true;
        else if ( cur_alert && sat_pct < ANLG_ALERT_HYST)      desired = false;
        else                                                     desired = cur_alert;

        // Debounce
        if (xSemaphoreTake(g51_anlg_mutex, portMAX_DELAY) == pdTRUE) {
            g51_anlg.raw_pot  = raw_pct;
            g51_anlg.saturated = sat_pct;
            g51_anlg.pwm_value = pwm_val;
            g51_anlg.level_pct = sat_pct;

            if (desired == g51_anlg.pending_alert) {
                g51_anlg.alert_bounce++;
            } else {
                g51_anlg.pending_alert = desired;
                g51_anlg.alert_bounce  = 1;
            }
            if (g51_anlg.alert_bounce >= ANLG_DEBOUNCE_SAMPLES) {
                g51_anlg.alert_active  = g51_anlg.pending_alert;
                g51_anlg.alert_bounce  = ANLG_DEBOUNCE_SAMPLES; // cap
            }

            xSemaphoreGive(g51_anlg_mutex);
        }

        // --- YELLOW LED: analog actuator active indicator ----------------
        if (sat_pct > 10) {
            dd_led_2_turn_on();   // YELLOW = dimmer active
        } else {
            dd_led_2_turn_off();  // YELLOW = off / idle
        }
        dd_led_apply();

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(75));
    }
}