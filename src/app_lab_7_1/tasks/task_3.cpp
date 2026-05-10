#include "task_3.h"
#include "task_config.h"
#include "srv_stdio_lcd/srv_stdio_lcd.h"
#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

// ===========================================================================
// task_3 – LCD + Serial output  (500 ms period, priority 2)
// ===========================================================================

#define SEM_TICKS pdMS_TO_TICKS(10)

// ---------------------------------------------------------------------------
// FNV-1a hash
// ---------------------------------------------------------------------------
static uint32_t fnv1a_snap(const App71Snapshot_t *s) {
    uint32_t h = 2166136261UL;
#define FNV_BYTE(b) h = (h ^ (uint8_t)(b)) * 16777619UL
    FNV_BYTE((uint8_t)s->fsm_state);
    FNV_BYTE(s->led_on    ? 1 : 0);
    FNV_BYTE(s->btn_stuck ? 1 : 0);
    FNV_BYTE((uint8_t)(s->press_count   & 0xFF));
    FNV_BYTE((uint8_t)((s->press_count  >> 8) & 0xFF));
    FNV_BYTE((uint8_t)(s->timeout_rem_s & 0xFF));
#undef FNV_BYTE
    return h;
}

// ---------------------------------------------------------------------------
// render_lcd – 16x2
//
// Row 0: "FSM  Lab 7.1    "
// Row 1:
//   LED_OFF:              "LED: OFF        "
//   LED_ON  + stuck:      "LED: ON   !ALT  "
//   LED_ON  + countdown:  "LED: ON   T-XXs "
// ---------------------------------------------------------------------------
static void render_lcd(const App71Snapshot_t *s) {
    lcd.clear();

    // Row 0
    lcd.setCursor(0, 0);
    lcd.print("FSM  Lab 7.1    ");

    // Row 1
    lcd.setCursor(0, 1);

    if (s->btn_stuck) {
        if (s->fsm_state == FSM_LED_ON)
            lcd.print("LED: ON   !ALT  ");
        else
            lcd.print("LED: OFF  !ALT  ");
    } else if (s->fsm_state == FSM_LED_ON) {
        // "LED: ON   T-XXs "  (T- = timeout countdown)
        lcd.print("LED: ON   T-");
        if (s->timeout_rem_s < 10) lcd.print(' ');
        lcd.print((unsigned long)s->timeout_rem_s);
        lcd.print("s ");
    } else {
        lcd.print("LED: OFF        ");
    }
}

// ---------------------------------------------------------------------------
// render_serial
// ---------------------------------------------------------------------------
static void render_serial(const App71Snapshot_t *s) {
    if (g_app71_io_mutex != NULL)
        if (xSemaphoreTake(g_app71_io_mutex, pdMS_TO_TICKS(50)) != pdTRUE) return;

    const char *state_str = (s->fsm_state == FSM_LED_ON) ? "LED_ON " : "LED_OFF";

    printf("\r\n[T=%lums] Lab 7.1 FSM ============\n", s->uptime_ms);
    printf("\r STATE  : %s  (presses: %lu)\n", state_str, s->press_count);
    printf("\r BUTTON : RAW=%d  DEB=%d  STUCK=%d\n",
           s->btn_raw ? 1 : 0,
           s->btn_debounced ? 1 : 0,
           s->btn_stuck ? 1 : 0);
    printf("\r LED    : [%s]\n", s->led_on ? "ON " : "OFF");

    if (s->fsm_state == FSM_LED_ON && s->timeout_rem_s > 0)
        printf("\r TIMEOUT: %lus remaining (auto-off)\n", s->timeout_rem_s);
    else if (s->fsm_state == FSM_LED_OFF)
        printf("\r TIMEOUT: inactive\n");

    if (s->btn_stuck)
        printf("\r ALERT  : [!!] Button held > %dms – check wiring\n",
               STUCK_THRESHOLD_MS);
    else
        printf("\r ALERT  : [OK]\n");

    printf("\r====================================\n");

    if (g_app71_io_mutex != NULL)
        xSemaphoreGive(g_app71_io_mutex);
}

// ---------------------------------------------------------------------------
// fetch_snapshot
// ---------------------------------------------------------------------------
static bool fetch_snapshot(App71Snapshot_t *out) {
    if (xSemaphoreTake(g_app71_snapshot_mutex, SEM_TICKS) != pdTRUE)
        return false;
    *out = g_app71_snapshot;
    xSemaphoreGive(g_app71_snapshot_mutex);
    return true;
}

// ---------------------------------------------------------------------------
// Task entry point
// ---------------------------------------------------------------------------
void task3_71_run(void *pvParameters) {
    (void)pvParameters;

    uint32_t   prev_hash      = 0;
    bool       have_prev      = false;
    TickType_t ticks_since_tx = 0;
    const TickType_t HB_TICKS = pdMS_TO_TICKS(SERIAL_HEARTBEAT_MS);

    for (;;) {
        App71Snapshot_t snap = {
            FSM_LED_OFF, false, false, false, false, 0, false, 0, 0
        };

        if (fetch_snapshot(&snap)) {
            render_lcd(&snap);

#if SERIAL_MODE_PLOTTER
            render_plotter(&snap);
#else
            uint32_t hash = fnv1a_snap(&snap);

            bool changed = !have_prev || (hash != prev_hash);
            bool hb_due  = have_prev  && (ticks_since_tx >= HB_TICKS);
            bool forced  = snap.force_report;

            if (changed || hb_due || forced) {
                render_serial(&snap);
                prev_hash      = hash;
                have_prev      = true;
                ticks_since_tx = 0;
            }
#endif
        }

        ticks_since_tx += pdMS_TO_TICKS(REPORT_PERIOD_MS);
        vTaskDelay(pdMS_TO_TICKS(REPORT_PERIOD_MS));
    }
}