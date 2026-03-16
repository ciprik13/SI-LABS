#include "task_2.h"
#include "dd_sns_temperature/dd_sns_temperature.h"
#include "dd_sns_dht/dd_sns_dht.h"
#include "dd_led/dd_led.h"
#include <Arduino_FreeRTOS.h>
#include <semphr.h>

// ===========================================================================
// Shared conditioning state – owned here, accessed externally via task_config.h
// ===========================================================================
CondState_t       g_cond        = { false, false, 0 };  // S1 analog
SemaphoreHandle_t g_cond_mutex  = NULL;

CondState_t       g_cond2       = { false, false, 0 };  // S2 digital DHT22
SemaphoreHandle_t g_cond2_mutex = NULL;

void task_2_init() {
    g_cond_mutex  = xSemaphoreCreateMutex();
    g_cond2_mutex = xSemaphoreCreateMutex();
}

// ===========================================================================
// Task 2 – Signal Conditioning / Threshold Alerting  (50 ms, +10 ms offset)
//
// State machine (per sensor):
//   OK          -> PENDING_ON  : temperature > THRESHOLD_HIGH
//   PENDING_ON  -> ALERT       : ANTIBOUNCE_SAMPLES consecutive confirmations
//   ALERT       -> PENDING_OFF : temperature < THRESHOLD_LOW
//   PENDING_OFF -> OK          : ANTIBOUNCE_SAMPLES consecutive confirmations
//
// Combined LED (most critical state wins):
//   RED    = at least one sensor in committed ALERT
//   YELLOW = at least one sensor PENDING (transitioning)
//   GREEN  = both sensors OK
// ===========================================================================
void task_conditioning(void *pvParameters) {
    (void) pvParameters;

    // 10 ms offset – ensures task_acquisition has already written a sample
    vTaskDelay(pdMS_TO_TICKS(10));
    TickType_t xLastWakeTime = xTaskGetTickCount();

    for (;;) {
        // ===== S1 – Potentiometer (analog) ==================================
        int temp1 = dd_sns_temperature_get_celsius();

        bool cur1 = false;
        if (xSemaphoreTake(g_cond_mutex, portMAX_DELAY) == pdTRUE) {
            cur1 = g_cond.alert_active;
            xSemaphoreGive(g_cond_mutex);
        }

        bool des1;
        if      (!cur1 && temp1 > ALERT_THRESHOLD_HIGH) des1 = true;
        else if ( cur1 && temp1 < ALERT_THRESHOLD_LOW)  des1 = false;
        else                                             des1 = cur1;

        bool committed1 = false, pending1 = false;
        if (xSemaphoreTake(g_cond_mutex, portMAX_DELAY) == pdTRUE) {
            if (des1 == g_cond.pending_state) g_cond.bounce_count++;
            else { g_cond.pending_state = des1; g_cond.bounce_count = 1; }
            if (g_cond.bounce_count >= ANTIBOUNCE_SAMPLES) {
                g_cond.alert_active = g_cond.pending_state;
                g_cond.bounce_count = ANTIBOUNCE_SAMPLES;
            }
            committed1 = g_cond.alert_active;
            pending1   = (g_cond.pending_state != g_cond.alert_active) &&
                         (g_cond.bounce_count > 0);
            xSemaphoreGive(g_cond_mutex);
        }

        // ===== S2 – DHT22 (digital) =========================================
        int temp2 = dd_sns_dht_get_celsius();

        bool cur2 = false;
        if (xSemaphoreTake(g_cond2_mutex, portMAX_DELAY) == pdTRUE) {
            cur2 = g_cond2.alert_active;
            xSemaphoreGive(g_cond2_mutex);
        }

        bool des2;
        if      (!cur2 && temp2 > ALERT2_THRESHOLD_HIGH) des2 = true;
        else if ( cur2 && temp2 < ALERT2_THRESHOLD_LOW)  des2 = false;
        else                                              des2 = cur2;

        bool committed2 = false, pending2 = false;
        if (xSemaphoreTake(g_cond2_mutex, portMAX_DELAY) == pdTRUE) {
            if (des2 == g_cond2.pending_state) g_cond2.bounce_count++;
            else { g_cond2.pending_state = des2; g_cond2.bounce_count = 1; }
            if (g_cond2.bounce_count >= ANTIBOUNCE2_SAMPLES) {
                g_cond2.alert_active = g_cond2.pending_state;
                g_cond2.bounce_count = ANTIBOUNCE2_SAMPLES;
            }
            committed2 = g_cond2.alert_active;
            pending2   = (g_cond2.pending_state != g_cond2.alert_active) &&
                         (g_cond2.bounce_count > 0);
            xSemaphoreGive(g_cond2_mutex);
        }

        // --- Combined LED indicator (most critical state of either sensor) --
        // RED    = at least one sensor in committed ALERT
        // YELLOW = at least one sensor PENDING (transitioning)
        // GREEN  = both sensors OK
        if (committed1 || committed2) {
            dd_led_turn_on();    // RED
            dd_led_1_turn_off(); // GREEN
            dd_led_2_turn_off(); // YELLOW
        } else if (pending1 || pending2) {
            dd_led_turn_off();   // RED
            dd_led_1_turn_off(); // GREEN
            dd_led_2_turn_on();  // YELLOW
        } else {
            dd_led_turn_off();   // RED
            dd_led_1_turn_on();  // GREEN
            dd_led_2_turn_off(); // YELLOW
        }
        dd_led_apply();          // commit targets to physical pins

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(50));
    }
}