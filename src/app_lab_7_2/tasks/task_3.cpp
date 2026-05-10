#include "task_3.h"
#include "task_config.h"
#include "srv_stdio_lcd/srv_stdio_lcd.h"
#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#define SEM_TICKS pdMS_TO_TICKS(10)

// ---------------------------------------------------------------------------
// FNV-1a hash for change detection
// ---------------------------------------------------------------------------
static uint32_t fnv1a_snap(const App72Snapshot_t *s) {
    uint32_t h = 2166136261UL;
#define FNV_BYTE(b) h = (h ^ (uint8_t)(b)) * 16777619UL
    FNV_BYTE((uint8_t)s->state);
    FNV_BYTE(s->ns_request  ? 1 : 0);
    FNV_BYTE(s->night_mode  ? 1 : 0);
    FNV_BYTE(s->night_blink ? 1 : 0);
    FNV_BYTE((uint8_t)(s->state_ms & 0xFF));
    FNV_BYTE((uint8_t)((s->state_ms >> 8) & 0xFF));
#undef FNV_BYTE
    return h;
}

// ---------------------------------------------------------------------------
// State name helper
// ---------------------------------------------------------------------------
static const char *state_name(TrafficState_t st) {
    switch (st) {
        case FSM_EV_GREEN:  return "EV_GREEN ";
        case FSM_EV_YELLOW: return "EV_YELLOW";
        case FSM_ALL_RED_1: return "ALL_RED_1";
        case FSM_NS_GREEN:  return "NS_GREEN ";
        case FSM_NS_YELLOW: return "NS_YELLOW";
        case FSM_ALL_RED_2: return "ALL_RED_2";
        case FSM_NIGHT:     return "NIGHT    ";
        default:            return "UNKNOWN  ";
    }
}

static const char *led_str(bool g, bool y, bool r) {
    if (g) return "GRN";
    if (y) return "YEL";
    if (r) return "RED";
    return "---";
}

// ---------------------------------------------------------------------------
// render_lcd
// ---------------------------------------------------------------------------
static void render_lcd(const App72Snapshot_t *s) {
    lcd.clear();

    if (s->night_mode) {
        lcd.setCursor(0, 0);
        lcd.print("** NIGHT MODE **");
        lcd.setCursor(0, 1);
        lcd.print("  Yellow blink  ");
        return;
    }

    // Row 0: "EV:GRN  NS:RED  "
    lcd.setCursor(0, 0);
    lcd.print("EV:");
    lcd.print(led_str(s->ev_green, s->ev_yellow, s->ev_red));
    lcd.print("  NS:");
    lcd.print(led_str(s->ns_green, s->ns_yellow, s->ns_red));
    lcd.print("  ");

    // Row 1: "T:XXXXms [REQ]  " or "T:XXXXms        "
    lcd.setCursor(0, 1);
    lcd.print("T:");
    // print state_ms, right-aligned in 4 chars
    uint32_t t = s->state_ms;
    if (t < 10)    lcd.print("   ");
    else if (t < 100)  lcd.print("  ");
    else if (t < 1000) lcd.print(" ");
    lcd.print((unsigned long)t);
    lcd.print("ms");
    if (s->ns_request) {
        lcd.print(" [REQ]");
    } else {
        lcd.print("      ");
    }
}

// ---------------------------------------------------------------------------
// render_serial
// ---------------------------------------------------------------------------
static void render_serial(const App72Snapshot_t *s) {
    if (g_app72_io_mutex != NULL)
        if (xSemaphoreTake(g_app72_io_mutex, pdMS_TO_TICKS(50)) != pdTRUE) return;

    printf("\r\n[T=%lums] Lab 7.2 Traffic =======\n", s->uptime_ms);
    printf("\r STATE  : %s  (T: %lums)\n", state_name(s->state), s->state_ms);
    printf("\r EV     : %s | NS : %s\n",
           led_str(s->ev_green, s->ev_yellow, s->ev_red),
           led_str(s->ns_green, s->ns_yellow, s->ns_red));
    printf("\r REQUEST: %s\n", s->ns_request ? "YES" : "NO ");
    printf("\r NIGHT  : %s\n", s->night_mode ? "ON " : "OFF");
    printf("\r================================\n");

    if (g_app72_io_mutex != NULL)
        xSemaphoreGive(g_app72_io_mutex);
}

// ---------------------------------------------------------------------------
// fetch_snapshot
// ---------------------------------------------------------------------------
static bool fetch_snapshot(App72Snapshot_t *out) {
    if (xSemaphoreTake(g_app72_snapshot_mutex, SEM_TICKS) != pdTRUE)
        return false;
    *out = g_app72_snapshot;
    xSemaphoreGive(g_app72_snapshot_mutex);
    return true;
}

// ---------------------------------------------------------------------------
// Task entry point
// ---------------------------------------------------------------------------
void task3_72_run(void *pvParameters) {
    (void)pvParameters;

    uint32_t         prev_hash      = 0;
    bool             have_prev      = false;
    TickType_t       ticks_since_tx = 0;
    const TickType_t HB_TICKS       = pdMS_TO_TICKS(SERIAL_HEARTBEAT_MS);

    for (;;) {
        App72Snapshot_t snap = {};

        if (fetch_snapshot(&snap)) {
            render_lcd(&snap);

#if SERIAL_MODE_PLOTTER
            render_plotter(&snap);
#else
            uint32_t hash = fnv1a_snap(&snap);
            bool changed  = !have_prev || (hash != prev_hash);
            bool hb_due   = have_prev  && (ticks_since_tx >= HB_TICKS);
            bool forced   = snap.force_report;

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