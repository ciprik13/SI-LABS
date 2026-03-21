#include "task_3.h"
#include "task_config.h"
#include "srv_stdio_lcd/srv_stdio_lcd.h"
#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <stdio.h>
#include <string.h>

#define MTX_TIMEOUT pdMS_TO_TICKS(10)

// ---------------------------------------------------------------------------
// Build a 10-char ASCII progress bar for a PWM value 0-255
// e.g. pwm=128 → "[=====     ]"
// ---------------------------------------------------------------------------
static void build_pwm_bar(int pwm, char *out, int out_len) {
    const int BAR_W = 10;
    int filled = (pwm * BAR_W) / 255;
    if (filled > BAR_W) filled = BAR_W;

    int pos = 0;
    out[pos++] = '[';
    for (int i = 0; i < BAR_W && pos < out_len - 2; i++) {
        out[pos++] = (i < filled) ? '=' : ' ';
    }
    out[pos++] = ']';
    out[pos]   = '\0';
}

// ---------------------------------------------------------------------------
static bool snapshot_changed(const App5Snapshot_t *a, const App5Snapshot_t *b) {
    return a->bin_requested        != b->bin_requested        ||
           a->bin_pending          != b->bin_pending          ||
           a->bin_state            != b->bin_state            ||
           a->analog_mode          != b->analog_mode          ||
           a->angle_deg            != b->angle_deg            ||
           a->analog_requested_pwm != b->analog_requested_pwm ||
           a->analog_applied_pwm   != b->analog_applied_pwm   ||
           a->analog_alert         != b->analog_alert;
}

// ===========================================================================
// Task 3 – Display & Reporting  (500 ms period, priority 1)
//
// LCD 16×2 layout:
//   Row 0: "RLY:ON  MOT:178 "   relay state + applied PWM
//   Row 1: "ALT:OK  ANG:+45d"   alert status + potentiometer angle
//
// Serial: printed only when snapshot changed OR heartbeat (10 s) due.
//   Format: timestamped block with ASCII PWM bar.
// ===========================================================================
void task51_display(void *pvParameters) {
    (void) pvParameters;

    App5Snapshot_t last_serial    = { false, false, false, ANALOG_MODE_AUTO,
                                      0, 0, 0, false };
    bool           has_last       = false;
    TickType_t     last_print_tick = 0;
    const TickType_t heartbeat    = pdMS_TO_TICKS(ACTUATOR_SERIAL_HEARTBEAT_MS);

    for (;;) {
        // --- Read snapshot (single mutex lock) ----------------------------
        App5Snapshot_t snap = { false, false, false, ANALOG_MODE_AUTO,
                                0, 0, 0, false };
        if (xSemaphoreTake(g_app5_snapshot_mutex, MTX_TIMEOUT) == pdTRUE) {
            snap = g_app5_snapshot;
            xSemaphoreGive(g_app5_snapshot_mutex);
        } else {
            printf("[WARN] snapshot mutex timeout\n");
        }

        // --- Derived labels -----------------------------------------------
        const char *rly_state = snap.bin_state    ? "ON " : "OFF";
        const char *alrt_lbl  = snap.analog_alert ? "!AL" : "OK ";
        const char *dbnc_lbl  = snap.bin_pending  ? "PND" : "STB";
        int         pwm       = snap.analog_applied_pwm;
        int         angle     = snap.angle_deg;

        // --- LCD update (every 500 ms) ------------------------------------
        // Row 0: "RLY:ON  MOT:178 "
        // Row 1: "ALT:OK  ANG:+45d"
        char row0[17] = {0};
        char row1[17] = {0};
        snprintf(row0, sizeof(row0), "RLY:%-3s MOT:%3d ", rly_state, pwm);
        snprintf(row1, sizeof(row1), "ALT:%-3s ANG:%+4d", alrt_lbl, angle);

        lcd.clear();
        lcd.setCursor(0, 0);  lcd.print(row0);
        lcd.setCursor(0, 1);  lcd.print(row1);

        // --- Serial: only on change or heartbeat --------------------------
        TickType_t now      = xTaskGetTickCount();
        bool changed        = !has_last || snapshot_changed(&snap, &last_serial);
        bool heartbeat_due  = has_last && ((now - last_print_tick) >= heartbeat);

        if (changed || heartbeat_due) {
            char bar[14] = {0};
            build_pwm_bar(pwm, bar, sizeof(bar));

            uint32_t ms = (uint32_t)(now * portTICK_PERIOD_MS);

            printf("\n[T=%lums] Lab 5.1 ================\n", ms);

            // Binary actuator block
            printf(" RELAY  : [%s] debounce:%s\n", rly_state, dbnc_lbl);

            // Analog actuator block with PWM bar
            printf(" MOTOR  : PWM=%3d/255 %s\n", pwm, bar);
            printf("          mode:%-4s  angle:%+4ddeg\n",
                   (snap.analog_mode == ANALOG_MODE_AUTO) ? "AUTO" : "MAN",
                   angle);
            printf("          req:%3d  applied:%3d\n",
                   snap.analog_requested_pwm, pwm);

            // Alert block
            if (snap.analog_alert) {
                printf(" ALERT  : [!!] ACTIVE  HI=%d LO=%d\n",
                       ANALOG_ALERT_HIGH, ANALOG_ALERT_LOW);
            } else {
                printf(" ALERT  : [OK]         HI=%d LO=%d\n",
                       ANALOG_ALERT_HIGH, ANALOG_ALERT_LOW);
            }

            printf("=================================\n");

            last_serial     = snap;
            has_last        = true;
            last_print_tick = now;
        }

        vTaskDelay(pdMS_TO_TICKS(ACTUATOR_REPORT_PERIOD_MS));
    }
}