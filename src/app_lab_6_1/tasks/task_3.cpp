#include "task_3.h"
#include "task_config.h"
#include "srv_stdio_lcd/srv_stdio_lcd.h"
#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

// ===========================================================================
// task_3 – LCD + Serial output  (500 ms period, priority 1)
//
// FNV-1a hash of snapshot drives change detection – serial printed only
// on change or heartbeat (same pattern as Lab 5.2).
// Arduino Serial Plotter line is printed every tick unconditionally.
// ===========================================================================

#define SEM_TICKS pdMS_TO_TICKS(10)

// ---------------------------------------------------------------------------
// FNV-1a hash
// ---------------------------------------------------------------------------
static uint32_t fnv1a_snap(const App61Snapshot_t *s) {
    uint32_t h = 2166136261UL;
#define FNV_BYTE(b) h = (h ^ (uint8_t)(b)) * 16777619UL
    FNV_BYTE((uint8_t)(s->temp_c & 0xFF));
    FNV_BYTE((uint8_t)(s->humidity & 0xFF));
    FNV_BYTE((uint8_t)(s->sp & 0xFF));
    FNV_BYTE((uint8_t)(s->hyst & 0xFF));
    FNV_BYTE(s->relay_on      ? 1 : 0);
    FNV_BYTE(s->relay_pending ? 1 : 0);
    FNV_BYTE(s->extreme_dev   ? 1 : 0);
#undef FNV_BYTE
    return h;
}

// ---------------------------------------------------------------------------
// render_lcd – 16×2
//
// Row 0: "T:XXC SP:YY H:ZZ"  (XX=temp, YY=setpoint, ZZ=ON/OF/EX)
// Row 1: "HU:XX%  HY:Z STS"  (XX=humidity, Z=hyst, STS=OK/AL)
// ---------------------------------------------------------------------------
static void render_lcd(const App61Snapshot_t *s) {
    lcd.clear();

    // temp_display: round temp_raw (×10) to nearest integer for LCD
    // e.g. raw=7 → 1°C (rounds up from 0.7), raw=30 → 3°C
    int temp_display = (s->temp_raw >= 0)
                       ? (s->temp_raw + 5) / 10
                       : (s->temp_raw - 5) / 10;

    // --- Row 0 ---
    lcd.setCursor(0, 0);
    lcd.print("T:");
    if (temp_display < 10 && temp_display >= 0) lcd.print(' ');
    lcd.print(temp_display);
    lcd.print("C SP:");
    if (s->sp < 10) lcd.print(' ');
    lcd.print(s->sp);
    lcd.print(' ');
    if (s->extreme_dev) {
        lcd.print("!!EX");
    } else {
        lcd.print("H:");
        lcd.print(s->relay_on ? "ON" : "OF");
    }

    // --- Row 1 ---
    lcd.setCursor(0, 1);
    lcd.print("HU:");
    if (s->humidity < 10) lcd.print(' ');
    lcd.print(s->humidity);
    lcd.print("%  HY:");
    lcd.print(s->hyst);
    lcd.print(' ');
    lcd.print(s->extreme_dev ? "AL" : "OK");
}

// ---------------------------------------------------------------------------
// render_serial – full status block (holds IO mutex)
// ---------------------------------------------------------------------------
static void render_serial(const App61Snapshot_t *s) {
    TickType_t long_timeout = pdMS_TO_TICKS(50);

    if (g_app61_io_mutex != NULL) {
        if (xSemaphoreTake(g_app61_io_mutex, long_timeout) != pdTRUE) return;
    }

    printf("\r\n[T=%lums] Lab 6.1 ================\n", s->uptime_ms);
    // Show 0.1°C resolution: temp_raw=7 → "0.7°C", temp_raw=30 → "3.0°C"
    int t_int  = s->temp_raw / 10;
    int t_frac = s->temp_raw < 0 ? -(s->temp_raw % 10) : s->temp_raw % 10;
    printf("\r TEMP   : %d.%d\xc2\xb0""C  (raw: %d)\n", t_int, t_frac, s->temp_raw);
    printf("\r HUMID  : %d %%RH\n", s->humidity);
    printf("\r SETPNT : SP=%d°C  HYST=%d°C\n", s->sp, s->hyst);
    printf("\r THRESH : ON < %d°C   OFF > %d°C\n", s->thresh_on, s->thresh_off);
    printf("\r RELAY  : [%s] debounce:%s\n",
           s->relay_on ? "ON " : "OFF",
           s->relay_pending ? "PND" : "STB");
    if (s->extreme_dev)
        printf("\r EXTREME: [!!] |T-SP| > %d*HYST=%d°C  CHECK SENSOR/HEATER\n",
               EXTREME_DEV_FACTOR, EXTREME_DEV_FACTOR * s->hyst);
    else
        printf("\r EXTREME: [OK]\n");
    printf("\r====================================\n");

    if (g_app61_io_mutex != NULL) {
        xSemaphoreGive(g_app61_io_mutex);
    }
}

// ---------------------------------------------------------------------------
// render_plotter – Arduino Serial Plotter compatible line
// Format: "SetPoint:<sp>\tValue:<temp>\tOutput:<0|1>"
// Printed every task tick (500 ms) regardless of change
// ---------------------------------------------------------------------------
static void render_plotter(const App61Snapshot_t *s) {
    TickType_t timeout = pdMS_TO_TICKS(10);

    if (g_app61_io_mutex != NULL) {
        if (xSemaphoreTake(g_app61_io_mutex, timeout) != pdTRUE) return;
    }

    // Serial Plotter: Value uses temp_raw/10 (rounded) so it plots on the
    // same integer °C scale as SetPoint. temp_raw=7 → 1, temp_raw=30 → 3.
    int temp_plot = (s->temp_raw >= 0)
                    ? (s->temp_raw + 5) / 10
                    : (s->temp_raw - 5) / 10;
    printf("\rSetPoint:%d\tValue:%d\tOutput:%d\n",
           s->sp, temp_plot, s->relay_on ? 1 : 0);

    if (g_app61_io_mutex != NULL) {
        xSemaphoreGive(g_app61_io_mutex);
    }
}

// ---------------------------------------------------------------------------
// fetch_snapshot
// ---------------------------------------------------------------------------
static bool fetch_snapshot(App61Snapshot_t *out) {
    if (xSemaphoreTake(g_app61_snapshot_mutex, SEM_TICKS) != pdTRUE) {
        return false;
    }
    *out = g_app61_snapshot;
    xSemaphoreGive(g_app61_snapshot_mutex);
    return true;
}

// ---------------------------------------------------------------------------
// Task entry point
// ---------------------------------------------------------------------------
void task3_61_run(void *pvParameters) {
    (void)pvParameters;

    lcd.init();
    lcd.backlight();
    lcd.clear();
    lcd.home();

    uint32_t   prev_hash      = 0;
    bool       have_prev      = false;
    TickType_t ticks_since_tx = 0;
    const TickType_t HB_TICKS = pdMS_TO_TICKS(SERIAL_HEARTBEAT_MS);

    for (;;) {
        App61Snapshot_t snap = {
            0, 0, 0,
            SP_DEFAULT, HYST_DEFAULT,
            SP_DEFAULT - HYST_DEFAULT / 2,
            SP_DEFAULT + HYST_DEFAULT / 2,
            false, false, false, false, 0
        };

        if (fetch_snapshot(&snap)) {
            // Skip until task_2 has published at least one real sensor read.
            // uptime_ms == 0 with temp_raw == 0 means the sensor gate has not
            // passed yet — printing T=0ms / 0°C is misleading.
            if (snap.uptime_ms == 0 && snap.temp_raw == 0) {
                ticks_since_tx += pdMS_TO_TICKS(REPORT_PERIOD_MS);
                vTaskDelay(pdMS_TO_TICKS(REPORT_PERIOD_MS));
                continue;
            }

            uint32_t hash = fnv1a_snap(&snap);

            // LCD: unconditional every tick
            render_lcd(&snap);

            // Serial Plotter: unconditional every tick
            render_plotter(&snap);

            // Serial block: on change, heartbeat, or STATUS command
            bool changed  = !have_prev || (hash != prev_hash);
            bool hb_due   = have_prev  && (ticks_since_tx >= HB_TICKS);
            bool forced   = snap.force_report;

            if (changed || hb_due || forced) {
                render_serial(&snap);
                prev_hash      = hash;
                have_prev      = true;
                ticks_since_tx = 0;
            }
        }

        ticks_since_tx += pdMS_TO_TICKS(REPORT_PERIOD_MS);
        vTaskDelay(pdMS_TO_TICKS(REPORT_PERIOD_MS));
    }
}