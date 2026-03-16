#include "task_2.h"
#include "task_config.h"
#include <Arduino_FreeRTOS.h>
#include <semphr.h>

// ===========================================================================
// Shared state definitions (owned here, declared extern in task_config.h)
// ===========================================================================
CmdState51_t      g51_cmd       = { -1, 0, false };
SemaphoreHandle_t g51_cmd_mutex = NULL;

BinActState51_t   g51_bin       = { -1, -1, 0, 0, false, false };
SemaphoreHandle_t g51_bin_mutex = NULL;

AnlgActState51_t  g51_anlg      = { 0, 0, 0, 0, false, false, 0 };
SemaphoreHandle_t g51_anlg_mutex = NULL;

// ------------------------------------------------------------------
// task51_init – called once from app_lab_5_1_setup()
// ------------------------------------------------------------------
void task51_init() {
    g51_cmd_mutex  = xSemaphoreCreateMutex();
    g51_bin_mutex  = xSemaphoreCreateMutex();
    g51_anlg_mutex = xSemaphoreCreateMutex();
}

// ===========================================================================
// Task 2 – Signal Conditioning  (50 ms period, priority 2)
//
// Binary actuator pipeline:
//   raw_cmd  →  saturation (clamp to {0,1})  →  debounce
//           →  g51_bin.committed  (read by task51_binary_ctrl)
//
// Saturation: any value outside {0,1} is discarded (treated as -1/none).
// Debounce  : the candidate value must arrive BIN_DEBOUNCE_SAMPLES times
//             in a row before it becomes the committed state.
// ===========================================================================
void task51_signal_cond(void *pvParameters) {
    (void) pvParameters;

    // Small offset so task_1 always runs first each cycle
    vTaskDelay(pdMS_TO_TICKS(5));
    TickType_t xLastWakeTime = xTaskGetTickCount();

    for (;;) {
        // --- 1. Snapshot the latest command from task_1 -------------------
        int raw_cmd = -1;

        if (xSemaphoreTake(g51_cmd_mutex, portMAX_DELAY) == pdTRUE) {
            raw_cmd = g51_cmd.raw_cmd;
            // Clear the command so we don't re-process it next tick
            g51_cmd.raw_cmd     = -1;
            g51_cmd.invalid_cmd = false;
            xSemaphoreGive(g51_cmd_mutex);
        }

        // --- 2. Saturation: only 0 or 1 are valid ------------------------
        // Values outside {0,1} are discarded (no state change)
        int saturated = raw_cmd;
        if (raw_cmd != 0 && raw_cmd != 1) {
            saturated = -1;   // nothing to do
        }

        // --- 3. Debounce: require BIN_DEBOUNCE_SAMPLES consistent ticks --
        if (saturated != -1) {
            if (xSemaphoreTake(g51_bin_mutex, portMAX_DELAY) == pdTRUE) {
                g51_bin.requested = saturated;

                if (saturated == g51_bin.pending) {
                    g51_bin.bounce_count++;
                } else {
                    g51_bin.pending      = saturated;
                    g51_bin.bounce_count = 1;
                }

                if (g51_bin.bounce_count >= BIN_DEBOUNCE_SAMPLES) {
                    g51_bin.committed    = g51_bin.pending;
                    g51_bin.bounce_count = BIN_DEBOUNCE_SAMPLES; // cap
                }

                xSemaphoreGive(g51_bin_mutex);
            }
        }

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(50));
    }
}