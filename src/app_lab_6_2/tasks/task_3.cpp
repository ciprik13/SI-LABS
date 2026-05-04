#include "task_3.h"
#include "task_config.h"
#include "srv_stdio_lcd/srv_stdio_lcd.h"
#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

// ===========================================================================
// task_3 – LCD + Serial output  (500 ms period, priority 1)
//
// FNV-1a hash of snapshot drives change detection – serial block printed
// only on change or 10 s heartbeat (same pattern as Lab 6.1).
// Serial Plotter line printed every tick unconditionally.
// ===========================================================================

#define SEM_TICKS pdMS_TO_TICKS(10)

// ---------------------------------------------------------------------------
// FNV-1a hash over key snapshot fields for change detection
// ---------------------------------------------------------------------------
static uint32_t fnv1a_snap(const App62Snapshot_t *s) {
    uint32_t h = 2166136261UL;
#define FNV_BYTE(b) h = (h ^ (uint8_t)(b)) * 16777619UL
    // Use integer-representation of floats for stable hash
    int t10 = (int)(s->temp_f * 10.0f);
    FNV_BYTE((uint8_t)(t10 & 0xFF));
    FNV_BYTE((uint8_t)(t10 >> 8));
    FNV_BYTE((uint8_t)(s->sp & 0xFF));
    FNV_BYTE((uint8_t)(s->humidity & 0xFF));
    FNV_BYTE((uint8_t)((int)s->pid_out & 0xFF));
    FNV_BYTE(s->relay_on    ? 1 : 0);
    FNV_BYTE(s->manual_mode ? 1 : 0);
    FNV_BYTE(s->alarm       ? 1 : 0);
#undef FNV_BYTE
    return h;
}

// ---------------------------------------------------------------------------
// render_lcd – 16×2
//
// Row 0: "T:XX.X SP:YY ZZ"   ZZ = AU / MA / AL
// Row 1: "OUT:XXX% E:+X.X "
// ---------------------------------------------------------------------------
static void render_lcd(const App62Snapshot_t *s) {
    lcd.clear();

    // Mode/alarm indicator (2 chars)
    const char *mode_str;
    if (s->alarm)             mode_str = "AL";
    else if (s->manual_mode)  mode_str = "MA";
    else                      mode_str = "AU";

    // --- Row 0 ---
    lcd.setCursor(0, 0);
    lcd.print("T:");
    // Print with one decimal: 23.4
    int t_int  = (int)s->temp_f;
    int t_frac = (int)(fabsf(s->temp_f - (float)t_int) * 10.0f + 0.5f);
    if (t_frac >= 10) { t_int++; t_frac = 0; }
    if (s->temp_f < 0 && t_int == 0) lcd.print('-');
    lcd.print(t_int);
    lcd.print('.');
    lcd.print(t_frac);
    lcd.print(" SP:");
    if (s->sp < 10) lcd.print(' ');
    lcd.print(s->sp);
    lcd.print(' ');
    lcd.print(mode_str);

    // --- Row 1 ---
    lcd.setCursor(0, 1);
    lcd.print("OUT:");
    int out_int = (int)(s->pid_out + 0.5f);
    if (out_int < 10)  lcd.print(' ');
    if (out_int < 100) lcd.print(' ');
    lcd.print(out_int);
    lcd.print("% E:");
    // Error with sign and one decimal
    float e = s->error;
    if (e >= 0) lcd.print('+');
    int e_int  = (int)e;
    int e_frac = (int)(fabsf(e - (float)e_int) * 10.0f + 0.5f);
    if (e_frac >= 10) {
        if (e >= 0) e_int++; else e_int--;
        e_frac = 0;
    }
    lcd.print(e_int);
    lcd.print('.');
    lcd.print(e_frac);
}

// ---------------------------------------------------------------------------
// render_serial – full PID status block (holds IO mutex)
// ---------------------------------------------------------------------------
static void render_serial(const App62Snapshot_t *s) {
    if (g_app62_io_mutex != NULL)
        if (xSemaphoreTake(g_app62_io_mutex, pdMS_TO_TICKS(50)) != pdTRUE) return;

    int t_int  = (int)s->temp_f;
    int t_frac = (int)(fabsf(s->temp_f - (float)t_int) * 10.0f + 0.5f);
    if (t_frac >= 10) { t_int++; t_frac = 0; }

    int e_abs_int  = (int)fabsf(s->error);
    int e_abs_frac = (int)((fabsf(s->error) - (float)e_abs_int) * 100.0f + 0.5f);

    int p_int = (int)fabsf(s->pid_p);
    int p_frac = (int)((fabsf(s->pid_p) - (float)p_int) * 100.0f + 0.5f);

    int i_int = (int)fabsf(s->pid_i);
    int i_frac = (int)((fabsf(s->pid_i) - (float)i_int) * 100.0f + 0.5f);

    int d_int = (int)fabsf(s->pid_d);
    int d_frac = (int)((fabsf(s->pid_d) - (float)d_int) * 100.0f + 0.5f);

    int out_int  = (int)s->pid_out;
    int out_frac = (int)((s->pid_out - (float)out_int) * 10.0f + 0.5f);

    // AVR printf does not support %f without -lprintf_flt linker flag.
    // All floats are printed using pre-computed int + frac pairs.
    int kp_int = (int)s->kp;
    int kp_frac = (int)((s->kp - (float)kp_int) * 100.0f + 0.5f);
    int ki_int = (int)s->ki;
    int ki_frac = (int)((s->ki - (float)ki_int) * 100.0f + 0.5f);
    int kd_int = (int)s->kd;
    int kd_frac = (int)((s->kd - (float)kd_int) * 100.0f + 0.5f);

    int al_int  = (int)fabsf(s->error);
    int al_frac = (int)((fabsf(s->error) - (float)al_int) * 10.0f + 0.5f);
    int thr_int  = (int)ALARM_THRESHOLD;
    int thr_frac = (int)((ALARM_THRESHOLD - (float)thr_int) * 10.0f + 0.5f);

    printf("\r\n[T=%lums] Lab 6.2 PID ============\n", s->uptime_ms);
    printf("\r TEMP   : %s%d.%d\xc2\xb0""C  (raw: %d)\n",
           (s->temp_f < 0 && t_int == 0) ? "-" : "",
           t_int, t_frac, s->temp_raw);
    printf("\r HUMID  : %d %%RH\n", s->humidity);
    printf("\r SETPNT : SP=%d\xc2\xb0""C\n", s->sp);
    printf("\r ERROR  : %s%d.%02d\xc2\xb0""C\n",
           s->error >= 0 ? "+" : "-", e_abs_int, e_abs_frac);
    printf("\r GAINS  : Kp=%d.%02d  Ki=%d.%02d  Kd=%d.%02d\n",
           kp_int, kp_frac, ki_int, ki_frac, kd_int, kd_frac);
    printf("\r PID    : P=%s%d.%02d  I=%s%d.%02d  D=%s%d.%02d\n",
           s->pid_p >= 0 ? "+" : "-", p_int, p_frac,
           s->pid_i >= 0 ? "+" : "-", i_int, i_frac,
           s->pid_d >= 0 ? "+" : "-", d_int, d_frac);
    printf("\r OUTPUT : %d.%d%%  [%s]\n",
           out_int, out_frac,
           s->manual_mode ? "MANUAL" : "AUTO");
    printf("\r RELAY  : [%s]\n", s->relay_on ? "ON " : "OFF");
    if (s->alarm)
        printf("\r ALARM  : [!!] |error|=%d.%d > %d.%d  CHECK SYSTEM\n",
               al_int, al_frac, thr_int, thr_frac);
    else
        printf("\r ALARM  : [OK]\n");
    printf("\r=====================================\n");

    if (g_app62_io_mutex != NULL)
        xSemaphoreGive(g_app62_io_mutex);
}

// ---------------------------------------------------------------------------
// render_plotter – Arduino Serial Plotter compatible line
// Format: "SetPoint:<sp>\tValue:<temp>\tOutput:<duty_int>"
// Printed every task tick (500 ms) unconditionally
// ---------------------------------------------------------------------------
static void render_plotter(const App62Snapshot_t *s) {
    if (g_app62_io_mutex != NULL)
        if (xSemaphoreTake(g_app62_io_mutex, pdMS_TO_TICKS(10)) != pdTRUE) return;

    int temp_plot = (s->temp_raw >= 0)
                    ? (s->temp_raw + 5) / 10
                    : (s->temp_raw - 5) / 10;
    int out_plot  = (int)(s->pid_out + 0.5f);

    printf("\rSetPoint:%d\tValue:%d\tOutput:%d\n",
           s->sp, temp_plot, out_plot);

    if (g_app62_io_mutex != NULL)
        xSemaphoreGive(g_app62_io_mutex);
}

// ---------------------------------------------------------------------------
// fetch_snapshot
// ---------------------------------------------------------------------------
static bool fetch_snapshot(App62Snapshot_t *out) {
    if (xSemaphoreTake(g_app62_snapshot_mutex, SEM_TICKS) != pdTRUE)
        return false;
    *out = g_app62_snapshot;
    xSemaphoreGive(g_app62_snapshot_mutex);
    return true;
}

// ---------------------------------------------------------------------------
// Task entry point
// ---------------------------------------------------------------------------
void task3_62_run(void *pvParameters) {
    (void)pvParameters;

    // LCD already initialised in app_lab_6_2_setup() before scheduler start.

    uint32_t   prev_hash      = 0;
    bool       have_prev      = false;
    TickType_t ticks_since_tx = 0;
    const TickType_t HB_TICKS = pdMS_TO_TICKS(SERIAL_HEARTBEAT_MS);

    for (;;) {
        App62Snapshot_t snap = {
            0, 0.0f, 0,
            SP_DEFAULT, KP_DEFAULT, KI_DEFAULT, KD_DEFAULT,
            0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
            false, false, 0.0f,
            false, false, 0
        };

        if (fetch_snapshot(&snap)) {
            // Skip until task_2 has published at least one real sensor read
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
            bool changed = !have_prev || (hash != prev_hash);
            bool hb_due  = have_prev  && (ticks_since_tx >= HB_TICKS);
            bool forced  = snap.force_report;

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