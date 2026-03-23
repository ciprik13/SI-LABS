#include "task_3.h"
#include "task_config.h"
#include "srv_stdio_lcd/srv_stdio_lcd.h"
#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

// ===========================================================================
// display_reporter – LCD + Serial output  (500 ms period, priority 1)
//
// Key structural decisions vs a naive implementation:
//
//  1. Change detection uses a 32-bit FNV-1a hash of the snapshot fields
//     instead of a field-by-field comparison struct.  Only one uint32_t
//     needs to be retained between ticks.
//
//  2. The snapshot is decoded into a flat DisplayVars_t (plain ints +
//     string pointers) before any output function is called.  LCD and
//     Serial functions receive only DisplayVars_t, never the raw snapshot
//     type — this decouples rendering from the shared data model.
//
//  3. The ASCII progress bar is produced by a function that fills a
//     caller-provided fixed-size buffer using only arithmetic (no loop
//     index compare against a computed value; fills from both ends).
//
//  4. Heartbeat tracking uses elapsed ticks stored as a TickType_t delta
//     rather than an absolute last-print timestamp, avoiding overflow
//     edge cases on long-running systems.
// ===========================================================================

#define SEM_TICKS pdMS_TO_TICKS(10)

// ---------------------------------------------------------------------------
// DisplayVars_t – decoded, render-ready values for one display tick
// ---------------------------------------------------------------------------
typedef struct {
    const char *relay_str;   // "ON " or "OFF"
    const char *alert_str;   // "!AL" or "OK "
    const char *dbnc_str;    // "PND" or "STB"
    const char *mode_str;    // "AUTO" or "MAN"
    int         pwm_out;     // applied PWM  [0..255]
    int         pwm_req;     // requested PWM
    int         pot_deg;     // potentiometer angle  [-135..+135]
    bool        alert_on;
    uint32_t    uptime_ms;
} DisplayVars_t;

// ---------------------------------------------------------------------------
// fnv1a_snap – 32-bit FNV-1a hash over the fields of an App5Snapshot_t
// Used for change detection without storing a full copy of the previous snap.
// ---------------------------------------------------------------------------
static uint32_t fnv1a_snap(const App5Snapshot_t *s) {
    uint32_t h = 2166136261UL;  // FNV offset basis

#define FNV_BYTE(b) h = (h ^ (uint8_t)(b)) * 16777619UL

    FNV_BYTE(s->bin_requested  ? 1 : 0);
    FNV_BYTE(s->bin_pending    ? 1 : 0);
    FNV_BYTE(s->bin_state      ? 1 : 0);
    FNV_BYTE((uint8_t)s->analog_mode);
    FNV_BYTE((uint8_t)(s->angle_deg         & 0xFF));
    FNV_BYTE((uint8_t)(s->angle_deg         >> 8));
    FNV_BYTE((uint8_t)(s->analog_requested_pwm & 0xFF));
    FNV_BYTE((uint8_t)(s->analog_applied_pwm   & 0xFF));
    FNV_BYTE(s->analog_alert   ? 1 : 0);

#undef FNV_BYTE
    return h;
}

// ---------------------------------------------------------------------------
// decode_snap – translate raw snapshot into render-ready DisplayVars_t
// ---------------------------------------------------------------------------
static DisplayVars_t decode_snap(const App5Snapshot_t *s, TickType_t now_ticks) {
    DisplayVars_t v;
    v.relay_str  = s->bin_state    ? "ON "  : "OFF";
    v.alert_str  = s->analog_alert ? "!AL"  : "OK ";
    v.dbnc_str   = s->bin_pending  ? "PND"  : "STB";
    v.mode_str   = (s->analog_mode == ANALOG_MODE_AUTO) ? "AUTO" : "MAN";
    v.pwm_out    = s->analog_applied_pwm;
    v.pwm_req    = s->analog_requested_pwm;
    v.pot_deg    = s->angle_deg;
    v.alert_on   = s->analog_alert;
    v.uptime_ms  = (uint32_t)((uint32_t)now_ticks * portTICK_PERIOD_MS);
    return v;
}

// ---------------------------------------------------------------------------
// fill_bar – write a 12-char "[==========]" bar into buf[0..11], null-term.
// Fills from the left with '=' and pads the remainder with ' '.
// buf must be at least 13 bytes.
// ---------------------------------------------------------------------------
static void fill_bar(int pwm, char *buf) {
    const int W = 10;
    int filled = (pwm * W) / 255;
    if (filled > W) filled = W;

    buf[0] = '[';
    for (int i = 0; i < W; i++)
        buf[1 + i] = (i < filled) ? '=' : ' ';
    buf[W + 1] = ']';
    buf[W + 2] = '\0';
}

// ---------------------------------------------------------------------------
// render_lcd – write both LCD rows from DisplayVars_t
// Row 0: "RLY:ON  MOT:178 "
// Row 1: "ALT:OK  ANG:+45d"
// ---------------------------------------------------------------------------
static void render_lcd(const DisplayVars_t *v) {
    char top[17], bot[17];
    snprintf(top, sizeof(top), "RLY:%-3s MOT:%3d ", v->relay_str, v->pwm_out);
    snprintf(bot, sizeof(bot), "ALT:%-3s ANG:%+4d", v->alert_str, v->pot_deg);

    lcd.clear();
    lcd.setCursor(0, 0); lcd.print(top);
    lcd.setCursor(0, 1); lcd.print(bot);
}

// ---------------------------------------------------------------------------
// render_serial – print full status block to Serial
// ---------------------------------------------------------------------------
static void render_serial(const DisplayVars_t *v) {
    char bar[13];
    fill_bar(v->pwm_out, bar);

    printf("\n[T=%lums] Lab 5.1 ================\n", v->uptime_ms);
    printf(" RELAY  : [%s] debounce:%s\n",   v->relay_str, v->dbnc_str);
    printf(" MOTOR  : PWM=%3d/255 %s\n",     v->pwm_out, bar);
    printf("          mode:%-4s  angle:%+4ddeg\n", v->mode_str, v->pot_deg);
    printf("          req:%3d  applied:%3d\n", v->pwm_req, v->pwm_out);

    if (v->alert_on)
        printf(" ALERT  : [!!] ACTIVE  HI=%d LO=%d\n",
               ANALOG_ALERT_HIGH, ANALOG_ALERT_LOW);
    else
        printf(" ALERT  : [OK]         HI=%d LO=%d\n",
               ANALOG_ALERT_HIGH, ANALOG_ALERT_LOW);

    printf("=================================\n");
}

// ---------------------------------------------------------------------------
// fetch_snapshot – copy shared snapshot under mutex; returns false on timeout
// ---------------------------------------------------------------------------
static bool fetch_snapshot(App5Snapshot_t *out) {
    if (xSemaphoreTake(g_app5_snapshot_mutex, SEM_TICKS) != pdTRUE) {
        printf("[WARN] snapshot mutex timeout\n");
        return false;
    }
    *out = g_app5_snapshot;
    xSemaphoreGive(g_app5_snapshot_mutex);
    return true;
}

// ---------------------------------------------------------------------------
// Task entry point
// ---------------------------------------------------------------------------
void display_reporter_run(void *pvParameters) {
    (void)pvParameters;

    uint32_t   prev_hash      = 0;
    bool       have_prev      = false;
    TickType_t ticks_since_tx = 0;
    const TickType_t HB_TICKS = pdMS_TO_TICKS(ACTUATOR_SERIAL_HEARTBEAT_MS);

    for (;;) {
        TickType_t     now  = xTaskGetTickCount();
        App5Snapshot_t snap = { false,false,false,ANALOG_MODE_AUTO,0,0,0,false };

        if (fetch_snapshot(&snap)) {
            DisplayVars_t v    = decode_snap(&snap, now);
            uint32_t      hash = fnv1a_snap(&snap);

            // LCD updated every tick unconditionally
            render_lcd(&v);

            // Serial: only on content change or heartbeat
            bool changed       = !have_prev || (hash != prev_hash);
            bool heartbeat_due = have_prev  && (ticks_since_tx >= HB_TICKS);

            if (changed || heartbeat_due) {
                render_serial(&v);
                prev_hash      = hash;
                have_prev      = true;
                ticks_since_tx = 0;
            }
        }

        ticks_since_tx += pdMS_TO_TICKS(ACTUATOR_REPORT_PERIOD_MS);
        vTaskDelay(pdMS_TO_TICKS(ACTUATOR_REPORT_PERIOD_MS));
    }
}