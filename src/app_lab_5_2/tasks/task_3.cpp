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
// LCD rows are designed to show the most operator-relevant info at a glance:
//
//   Row 0 priority: ALERT > LIMIT > normal state
//   Row 1 priority: shows mode + potentiometer (AUTO) or bar (MANUAL)
//
// FNV-1a hash of the snapshot fields drives change detection so only one
// uint32_t is stored between ticks.
// ===========================================================================

#define SEM_TICKS pdMS_TO_TICKS(10)

// ---------------------------------------------------------------------------
// App52DisplayVars_t
// ---------------------------------------------------------------------------
typedef struct {
    const char  *relay_str;   // "ON " / "OFF"
    const char  *dbnc_str;    // "STB" / "PND"
    const char  *mode_str;    // "AUTO" / "MAN "
    const char  *alert_str;   // "OK " / "!AL"
    int          pot_raw;     // 0-1023
    int          pwm_raw;     // pipeline input
    int          pwm_sat;
    int          pwm_med;
    int          pwm_wgt;
    int          pwm_rmp;     // applied
    bool         alert_on;
    bool         at_max;
    bool         at_min;
    bool         is_auto;
    uint32_t     uptime_ms;
} App52DisplayVars_t;

// ---------------------------------------------------------------------------
// FNV-1a hash
// ---------------------------------------------------------------------------
static uint32_t fnv1a_snap(const App52Snapshot_t *s) {
    uint32_t h = 2166136261UL;
#define FNV_BYTE(b) h = (h ^ (uint8_t)(b)) * 16777619UL
    FNV_BYTE(s->bin_requested ? 1 : 0);
    FNV_BYTE(s->bin_pending   ? 1 : 0);
    FNV_BYTE(s->bin_state     ? 1 : 0);
    FNV_BYTE((uint8_t)s->analog_mode);
    FNV_BYTE((uint8_t)(s->pot_raw   & 0xFF));
    FNV_BYTE((uint8_t)(s->pot_raw   >> 8));
    FNV_BYTE((uint8_t)(s->pwm_ramped & 0xFF));
    FNV_BYTE(s->motor_alert   ? 1 : 0);
    FNV_BYTE(s->at_limit_max  ? 1 : 0);
    FNV_BYTE(s->at_limit_min  ? 1 : 0);
#undef FNV_BYTE
    return h;
}

// ---------------------------------------------------------------------------
// decode_snap
// ---------------------------------------------------------------------------
static App52DisplayVars_t decode_snap(const App52Snapshot_t *s) {
    App52DisplayVars_t v;
    v.relay_str = s->bin_state   ? "ON " : "OFF";
    v.dbnc_str  = s->bin_pending ? "PND" : "STB";
    v.mode_str  = (s->analog_mode == ANALOG_MODE_AUTO) ? "AUTO" : "MAN ";
    v.alert_str = s->motor_alert ? "!AL" : "OK ";
    v.pot_raw   = s->pot_raw;
    v.pwm_raw   = s->pwm_raw;
    v.pwm_sat   = s->pwm_saturated;
    v.pwm_med   = s->pwm_median;
    v.pwm_wgt   = s->pwm_weighted;
    v.pwm_rmp   = s->pwm_ramped;
    v.alert_on  = s->motor_alert;
    v.at_max    = s->at_limit_max;
    v.at_min    = s->at_limit_min;
    v.is_auto   = (s->analog_mode == ANALOG_MODE_AUTO);
    v.uptime_ms = s->uptime_ms;
    return v;
}

// ---------------------------------------------------------------------------
// fill_bar – 10-char progress bar "[==========]" based on 0-255 value
// buf must be >= 13 bytes
// ---------------------------------------------------------------------------
static void fill_bar(int pwm, char *buf) {
    const int W = 10;
    int filled = (pwm * W) / 255;
    if (filled > W) filled = W;
    if (filled < 0) filled = 0;
    buf[0] = '[';
    for (int i = 0; i < W; i++)
        buf[1 + i] = (i < filled) ? '=' : ' ';
    buf[W + 1] = ']';
    buf[W + 2] = '\0';
}

// ---------------------------------------------------------------------------
// render_lcd – 16x2
//
// Row 0 (16 chars) – priority: ALERT > LIMIT > normal
//   Normal:    "RLY:ON  PWM: 178"
//   Alert:     "RLY:ON  **ALERT*"
//   Limit max: "RLY:OFF PWM: MAX"
//   Limit min: "RLY:OFF PWM:STOP"
//
// Row 1 (16 chars) – mode info
//   AUTO:      "AUTO POT:0512   "
//   MANUAL:    "MAN  [======]   "  (8-char bar inside brackets)
// ---------------------------------------------------------------------------
static void render_lcd(const App52DisplayVars_t *v) {
    char row0[17], row1[17];

    // --- Row 0 ---
    if (v->alert_on) {
        snprintf(row0, sizeof(row0), "RLY:%-3s **ALERT*", v->relay_str);
    } else if (v->at_max) {
        snprintf(row0, sizeof(row0), "RLY:%-3s PWM: MAX", v->relay_str);
    } else if (v->at_min) {
        snprintf(row0, sizeof(row0), "RLY:%-3s PWM:STOP", v->relay_str);
    } else {
        snprintf(row0, sizeof(row0), "RLY:%-3s PWM: %3d", v->relay_str, v->pwm_rmp);
    }

    // --- Row 1 ---
    if (v->is_auto) {
        // AUTO POT:0512   (shows raw ADC value)
        snprintf(row1, sizeof(row1), "AUTO POT:%4d   ", v->pot_raw);
    } else {
        // MAN  [========]  (8-char bar)
        int filled8 = (v->pwm_rmp * 8) / 255;
        if (filled8 > 8) filled8 = 8;
        char mini[11] = "[        ]";
        for (int i = 0; i < filled8; i++) mini[1 + i] = '=';
        snprintf(row1, sizeof(row1), "MAN  %-10s", mini);
    }

    lcd.clear();
    lcd.setCursor(0, 0); lcd.print(row0);
    lcd.setCursor(0, 1); lcd.print(row1);
}

// ---------------------------------------------------------------------------
// render_serial
// ---------------------------------------------------------------------------
static void render_serial(const App52DisplayVars_t *v) {
    char bar[13];
    fill_bar(v->pwm_rmp, bar);

    printf("\n[T=%lums] Lab 5.2 ================\n", v->uptime_ms);

    printf(" RELAY  : [%s] debounce:%s\n",
           v->relay_str, v->dbnc_str);

    if (v->is_auto)
        printf(" MODE   : AUTO    pot:%4d (map:%3d)\n",
               v->pot_raw, v->pwm_raw);
    else
        printf(" MODE   : MANUAL  pwm_set:%3d\n", v->pwm_raw);

    printf(" MOTOR  : raw:%3d sat:%3d med:%3d wgt:%3d\n",
           v->pwm_raw, v->pwm_sat, v->pwm_med, v->pwm_wgt);
    printf("          ramp:%3d  applied:%3d/255\n",
           v->pwm_rmp, v->pwm_rmp);
    printf("          %s\n", bar);

    if (v->alert_on)
        printf(" ALERT  : [!!] OVER-SPEED ACTIVE  HI=%d LO=%d\n",
               ALERT_HIGH_PWM, ALERT_LOW_PWM);
    else
        printf(" ALERT  : [OK]                    HI=%d LO=%d\n",
               ALERT_HIGH_PWM, ALERT_LOW_PWM);

    // Bonus limit advisory
    if (v->at_max)
        printf(" LIMIT  : [MAX] PWM=255 - use DEC to reduce speed\n");
    else if (v->at_min)
        printf(" LIMIT  : [MIN] PWM=0   - motor stopped\n");
    else
        printf(" LIMIT  : max:NO  min:NO\n");

    printf("====================================\n");
}

// ---------------------------------------------------------------------------
// fetch_snapshot
// ---------------------------------------------------------------------------
static bool fetch_snapshot(App52Snapshot_t *out) {
    if (xSemaphoreTake(g_app52_snapshot_mutex, SEM_TICKS) != pdTRUE) {
        printf("[WARN] snapshot mutex timeout\n");
        return false;
    }
    *out = g_app52_snapshot;
    xSemaphoreGive(g_app52_snapshot_mutex);
    return true;
}

// ---------------------------------------------------------------------------
// Task entry point
// ---------------------------------------------------------------------------
void task3_run(void *pvParameters) {
    (void)pvParameters;

    uint32_t   prev_hash      = 0;
    bool       have_prev      = false;
    TickType_t ticks_since_tx = 0;
    const TickType_t HB_TICKS = pdMS_TO_TICKS(SERIAL_HEARTBEAT_MS);

    for (;;) {
        App52Snapshot_t snap = {
            false,false,false,
            ANALOG_MODE_AUTO, 0, 0, 0, 0, 0, 0,
            false, false, false, 0
        };

        if (fetch_snapshot(&snap)) {
            App52DisplayVars_t v    = decode_snap(&snap);
            uint32_t      hash = fnv1a_snap(&snap);

            // LCD: unconditional every tick
            render_lcd(&v);

            // Serial: on change or heartbeat
            bool changed = !have_prev || (hash != prev_hash);
            bool hb_due  = have_prev  && (ticks_since_tx >= HB_TICKS);

            if (changed || hb_due) {
                render_serial(&v);
                prev_hash      = hash;
                have_prev      = true;
                ticks_since_tx = 0;
            }
        }

        ticks_since_tx += pdMS_TO_TICKS(REPORT_PERIOD_MS);
        vTaskDelay(pdMS_TO_TICKS(REPORT_PERIOD_MS));
    }
}