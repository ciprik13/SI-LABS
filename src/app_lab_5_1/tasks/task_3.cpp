#include "task_3.h"
#include "task_config.h"
#include "dd_led/dd_led.h"
#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <Arduino.h>

// ===========================================================================
// Task 3 – Binary Actuator Control  (75 ms period, priority 2)
//
// Reads the committed (debounced) binary command from g51_bin.committed
// and drives BINARY_ACT_PIN accordingly.
//
// Configurable recurrence: 75 ms (within the required 50–100 ms window).
//
// GREEN LED mirrors the actuator state for visual feedback.
// ===========================================================================

// Internal copy of last applied state (for actuator_get_state())
static volatile int s_state = 0;

int binary_actuator_get_state() {
    return s_state;
}

void task51_binary_ctrl(void *pvParameters) {
    (void) pvParameters;

    // Initialise pin as output
    pinMode(BINARY_ACT_PIN, OUTPUT);
    digitalWrite(BINARY_ACT_PIN, LOW);

    // Offset so conditioning always runs before control
    vTaskDelay(pdMS_TO_TICKS(15));
    TickType_t xLastWakeTime = xTaskGetTickCount();

    for (;;) {
        // --- Read committed state ------------------------------------------
        int committed = -1;

        if (xSemaphoreTake(g51_bin_mutex, portMAX_DELAY) == pdTRUE) {
            committed = g51_bin.committed;
            xSemaphoreGive(g51_bin_mutex);
        }

        // --- Apply to hardware --------------------------------------------
        if (committed == 1) {
            digitalWrite(BINARY_ACT_PIN, HIGH);
            s_state = 1;

            // Update GREEN LED (actuator ON indicator)
            dd_led_1_turn_on();   // GREEN = ON
        } else if (committed == 0) {
            digitalWrite(BINARY_ACT_PIN, LOW);
            s_state = 0;

            dd_led_1_turn_off();  // GREEN = OFF
        }
        // committed == -1 means no command received yet; keep current state

        dd_led_apply();

        // --- Write back actuator_on flag ----------------------------------
        if (xSemaphoreTake(g51_bin_mutex, portMAX_DELAY) == pdTRUE) {
            g51_bin.actuator_on = (s_state == 1);
            xSemaphoreGive(g51_bin_mutex);
        }

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(75));
    }
}