#include "task_3.h"
#include "task_config.h"
#include "dd_sns_temperature/dd_sns_temperature.h"
#include "srv_stdio_lcd/srv_stdio_lcd.h"
#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <stdio.h>

// ESC code understood by srv_stdio_lcd to clear display and home cursor
#define LCD_CLEAR "\x1b"

// ===========================================================================
// Task 3 – Display & Reporting  (2 000 ms)
//
// Writes a compact report to the LCD via fprintf (lcd stream).
// Writes the full structured report to Serial via printf (stdout).
// Both outputs are updated every 2 s (readable in terminal).
// Acquisition (task_1) and conditioning (task_2) still run at 50 ms.
// ===========================================================================
void task_report(void *pvParameters) {
    (void) pvParameters;

    for (;;) {
        int temperature = dd_sns_temperature_get_celsius();
        int raw         = dd_sns_temperature_get_raw();
        int voltage     = dd_sns_temperature_get_voltage();

        CondState_t snap = { false, false, 0 };
        if (xSemaphoreTake(g_cond_mutex, portMAX_DELAY) == pdTRUE) {
            snap = g_cond;
            xSemaphoreGive(g_cond_mutex);
        }

        const char *status = snap.alert_active  ? "[ALT]" :
                             snap.pending_state ? "[PND]" : " [OK]";

        // --- LCD: compact 16x2 layout ----------------------------------------
        FILE *lcd = srv_stdio_lcd_get_stream();
        if (lcd) {
            fprintf(lcd, LCD_CLEAR);
            fprintf(lcd, "Temp:%3dC %s", temperature, status);
            fprintf(lcd, "\nThr:%dC Bnc:%d/%d",
                    snap.alert_active ? ALERT_THRESHOLD_LOW : ALERT_THRESHOLD_HIGH,
                    snap.bounce_count, ANTIBOUNCE_SAMPLES);
        }

        // --- Serial: full structured report (visible in terminal) -------------
        printf("\r==============================\n");
        printf("\r [ACQUISITION]\n");
        printf("\r  RAW:     %4d ADC\n", raw);
        printf("\r  Voltage: %4d mV\n",  voltage);
        printf("\r  Temp:    %4d C\n",   temperature);
        printf("\r------------------------------\n");
        printf("\r [CONDITIONING]\n");
        printf("\r  Thr HIGH: %d C\n",     ALERT_THRESHOLD_HIGH);
        printf("\r  Thr LOW:  %d C\n",     ALERT_THRESHOLD_LOW);
        printf("\r  Bounce:   %d / %d\n",  snap.bounce_count, ANTIBOUNCE_SAMPLES);
        printf("\r  Pending:  %s\n",        snap.pending_state ? "ALERT" : "OK");
        printf("\r------------------------------\n");
        printf("\r [STATUS]:  %s\n",        snap.alert_active ? "!! ALERT !!" : "OK");
        if (snap.alert_active) {
            printf("\r >> WARNING: Temp exceeded %d C!\n", ALERT_THRESHOLD_HIGH);
        }
        printf("\r==============================\n\n");

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}