#include "task_2.h"
#include "dd_sns_temperature/dd_sns_temperature.h"
#include "dd_led/dd_led.h"
#include <Arduino_FreeRTOS.h>
#include <semphr.h>

// ===========================================================================
// Shared conditioning state – owned here, accessed externally via task_config.h
// ===========================================================================
CondState_t       g_cond       = { false, false, 0 };
SemaphoreHandle_t g_cond_mutex = NULL;

void task_2_init() {
    g_cond_mutex = xSemaphoreCreateMutex();
}

// ===========================================================================
// Task 2 – Signal Conditioning / Threshold Alerting  (50 ms, +10 ms offset)
//
// State machine:
//   OK          -> PENDING_ON  : temperature > THRESHOLD_HIGH
//   PENDING_ON  -> ALERT       : ANTIBOUNCE_SAMPLES consecutive confirmations
//   ALERT       -> PENDING_OFF : temperature < THRESHOLD_LOW
//   PENDING_OFF -> OK          : ANTIBOUNCE_SAMPLES consecutive confirmations
//
// Pending counter resets on any contradicting sample (antibounce).
// LED mirrors committed state: RED=alert, YELLOW=pending, GREEN=ok.
// ===========================================================================
void task_conditioning(void *pvParameters) {
    (void) pvParameters;

    // 10 ms offset – ensures task_acquisition has already written a sample
    vTaskDelay(pdMS_TO_TICKS(10));
    TickType_t xLastWakeTime = xTaskGetTickCount();

    for (;;) {
        int temperature = dd_sns_temperature_get_celsius();

        // --- Read current committed state -----------------------------------
        bool current_alert = false;
        if (xSemaphoreTake(g_cond_mutex, portMAX_DELAY) == pdTRUE) {
            current_alert = g_cond.alert_active;
            xSemaphoreGive(g_cond_mutex);
        }

        // --- Apply hysteresis to derive desired next state ------------------
        bool desired;
        if (!current_alert && temperature > ALERT_THRESHOLD_HIGH) {
            desired = true;           // rose above high threshold -> want ALERT
        } else if (current_alert && temperature < ALERT_THRESHOLD_LOW) {
            desired = false;          // fell below low threshold  -> want OK
        } else {
            desired = current_alert;  // inside hysteresis band    -> no change
        }

        // --- Antibounce: commit only after N stable samples ------------------
        bool committed = false;
        bool pending   = false;

        if (xSemaphoreTake(g_cond_mutex, portMAX_DELAY) == pdTRUE) {
            if (desired == g_cond.pending_state) {
                g_cond.bounce_count++;
            } else {
                g_cond.pending_state = desired;
                g_cond.bounce_count  = 1;
            }

            if (g_cond.bounce_count >= ANTIBOUNCE_SAMPLES) {
                g_cond.alert_active = g_cond.pending_state;
                g_cond.bounce_count = ANTIBOUNCE_SAMPLES; // clamp
            }

            committed = g_cond.alert_active;
            pending   = (g_cond.pending_state != g_cond.alert_active) &&
                        (g_cond.bounce_count > 0);
            xSemaphoreGive(g_cond_mutex);
        }

        // --- Visual LED indicator (latency < 100 ms) ------------------------
        // RED    = temperature alert committed and active
        // YELLOW = pending / debouncing (transitioning)
        // GREEN  = temperature within safe range
        if (committed) {
            dd_led_turn_on();    // RED
            dd_led_1_turn_off(); // GREEN
            dd_led_2_turn_off(); // YELLOW
        } else if (pending) {
            dd_led_turn_off();   // RED
            dd_led_1_turn_off(); // GREEN
            dd_led_2_turn_on();  // YELLOW
        } else {
            dd_led_turn_off();   // RED
            dd_led_1_turn_on();  // GREEN
            dd_led_2_turn_off(); // YELLOW
        }

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(50));
    }
}