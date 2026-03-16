#include "task_5.h"
#include "task_config.h"
#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <stdio.h>

// ===========================================================================
// Task 5 – Display & Reporting  (500 ms period, priority 1)
//
// Alternates between two LCD pages every 500 ms call:
//   Page 0: Binary actuator  (relay / LED state, debounce progress)
//   Page 1: Analog actuator  (dimmer level %, PWM value, alert)
//
// Serial report: full structured dump of both actuators.
// Invalid command: displayed as "!! INVALID CMD !!" for one full cycle.
// ===========================================================================

void task51_display(void *pvParameters) {
    (void) pvParameters;

    static uint8_t s_page       = 0;
    static bool    s_show_error = false;

    for (;;) {
        // --- Snapshot binary state ----------------------------------------
        BinActState51_t bin;
        if (xSemaphoreTake(g51_bin_mutex, portMAX_DELAY) == pdTRUE) {
            bin = g51_bin;
            xSemaphoreGive(g51_bin_mutex);
        }

        // --- Snapshot analog state ----------------------------------------
        AnlgActState51_t anlg;
        if (xSemaphoreTake(g51_anlg_mutex, portMAX_DELAY) == pdTRUE) {
            anlg = g51_anlg;
            xSemaphoreGive(g51_anlg_mutex);
        }

        // --- Check for invalid command (from cmd state) -------------------
        bool invalid_now = false;
        char cmd_src     = 's';
        if (xSemaphoreTake(g51_cmd_mutex, portMAX_DELAY) == pdTRUE) {
            invalid_now = g51_cmd.invalid_cmd;
            cmd_src     = g51_cmd.source;
            xSemaphoreGive(g51_cmd_mutex);
        }
        if (invalid_now) s_show_error = true;

        // --- Build human-readable labels ----------------------------------
        const char *bin_state  = bin.actuator_on  ? "ON " : "OFF";
        const char *bin_req    = (bin.requested == 1) ? "ON " :
                                 (bin.requested == 0) ? "OFF" : "---";
        const char *anlg_alert = anlg.alert_active  ? "[ALT]" : " [OK]";
        const char *src_label  = "serial";
        (void) cmd_src;  // only Serial is used in this variant

        // Debounce progress label "DBx/N" where x = bounce_count, N = max
        char db_buf[8];
        snprintf(db_buf, sizeof(db_buf), "DB%d/%d",
                 bin.bounce_count, BIN_DEBOUNCE_SAMPLES);

        // ==================================================================
        // LCD output (via printf tee to LCD + Serial)
        // ==================================================================
        printf("\x1b");   // clear LCD escape code (handled by srv_stdio_lcd)

        if (s_show_error) {
            // --- Error page (displayed for one cycle) ---------------------
            printf("!! INVALID CMD !!\n");
            printf("Use: 1/0/ON/OFF   \n");
            s_show_error = false;

        } else if (s_page == 0) {
            // --- Page 0: Binary actuator ----------------------------------
            // Row 0: "RELAY: ON  [DB2/3]"  (max 16 chars)
            printf("RELAY:%-3s [%s]\n", bin_state, db_buf);
            // Row 1: "CMD:ON  src:serial"
            printf("CMD:%-3s src:%-6s\n", bin_req, src_label);

        } else {
            // --- Page 1: Analog actuator ----------------------------------
            // Row 0: "DIM:  75%  [OK] "
            printf("DIM: %3d%% %s\n", anlg.level_pct, anlg_alert);
            // Row 1: "PWM:191 Pot: 75%"
            printf("PWM:%-3d Pot:%3d%%\n", anlg.pwm_value, anlg.raw_pot);
        }

        s_page = (s_page + 1) % 2;

        // ==================================================================
        // Serial-only full structured report
        // ==================================================================
        printf("\r==============================\n");
        printf("\r  Lab 5.1 – Actuator Control  \n");
        printf("\r==============================\n");

        // --- Binary actuator ---
        printf("\r [BINARY ACTUATOR – LED/Relay (pin %d)]\n", BINARY_ACT_PIN);
        printf("\r  State (committed) : %s\n",   bin.actuator_on ? "ON" : "OFF");
        printf("\r  Last command      : %s (src: %s)\n",
               (bin.requested == 1) ? "ON" : (bin.requested == 0) ? "OFF" : "none",
               src_label);
        printf("\r  Debounce          : %d / %d samples\n",
               bin.bounce_count, BIN_DEBOUNCE_SAMPLES);
        printf("\r  Pending           : %s\n",
               (bin.pending == 1) ? "ON" : (bin.pending == 0) ? "OFF" : "---");
        printf("\r  Error             : %s\n", bin.in_error ? "YES" : "NO");
        printf("\r------------------------------\n");

        // --- Analog actuator ---
        printf("\r [ANALOG ACTUATOR – PWM Dimmer (pin %d)]\n", ANALOG_ACT_PIN);
        printf("\r  Potentiometer     : %3d %%\n",    anlg.raw_pot);
        printf("\r  Saturated level   : %3d %%  [%d..%d %%]\n",
               anlg.saturated, ANLG_SAT_LOW, ANLG_SAT_HIGH);
        printf("\r  PWM value         : %3d / 255\n", anlg.pwm_value);
        printf("\r  Alert threshold   : %3d %%  hysteresis: %d %%\n",
               ANLG_ALERT_THRESHOLD, ANLG_ALERT_HYST);
        printf("\r  Alert debounce    : %d / %d samples\n",
               anlg.alert_bounce, ANLG_DEBOUNCE_SAMPLES);
        printf("\r  Alert (committed) : %s\n",
               anlg.alert_active ? "!! ALERT !!" : "OK");
        printf("\r==============================\n");

        // --- Invalid command warning (serial only) ---
        if (invalid_now) {
            printf("\r !! WARNING: INVALID COMMAND received from %s !!\n", src_label);
            printf("\r    Accepted commands: 1 / 0 / ON / OFF / STATUS\n");
            printf("\r------------------------------\n");
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}