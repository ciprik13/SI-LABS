#include "task_3.h"
#include "task_config.h"
#include "dd_sns_temperature/dd_sns_temperature.h"
#include "dd_sns_dht/dd_sns_dht.h"
#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <stdio.h>

// ===========================================================================
// Task 3 – Display & Reporting  (500 ms)
//
// LCD (16×2) layout:
//   Row 0:  "S1:RRR>WWW C [ST]"   RRR=raw, WWW=weighted (conditioned), ST=status
//   Row 1:  "S2:RRR>WWW C [ST]"
//
// Serial (via printf tee):
//   Full structured report with all four conditioning stages per sensor:
//     raw → saturated → median → weighted avg
//   Plus thresholds, debounce counter, and final alert status.
// ===========================================================================
void task42_report(void *pvParameters) {
    (void) pvParameters;

    for (;;) {
        // --- Snapshot S1 (analog potentiometer) ----------------------------
        CondFull42_t s1 = { 0, 0, 0, 0, false, false, 0 };
        if (xSemaphoreTake(g42_cond1_mutex, portMAX_DELAY) == pdTRUE) {
            s1 = g42_cond1;
            xSemaphoreGive(g42_cond1_mutex);
        }
        int voltage1 = dd_sns_temperature_get_voltage();
        int adc1     = dd_sns_temperature_get_raw();

        // --- Snapshot S2 (digital DHT22) -----------------------------------
        CondFull42_t s2 = { 0, 0, 0, 0, false, false, 0 };
        if (xSemaphoreTake(g42_cond2_mutex, portMAX_DELAY) == pdTRUE) {
            s2 = g42_cond2;
            xSemaphoreGive(g42_cond2_mutex);
        }
        int humidity = dd_sns_dht_get_humidity();
        int raw_dht  = dd_sns_dht_get_raw();  // temp × 10

        // Alert / pending status labels
        const char *st1 = s1.alert_active ? "[ALT]"
                        : (s1.pending_state != s1.alert_active && s1.bounce_count > 0)
                            ? "[PND]" : " [OK]";
        const char *st2 = s2.alert_active ? "[ALT]"
                        : (s2.pending_state != s2.alert_active && s2.bounce_count > 0)
                            ? "[PND]" : " [OK]";

        // ---------------------------------------------------------------
        // LCD rows (also forwarded to Serial via tee)
        // Format fits in 16 chars: "S1:RRR>WWW C STA" → 16 chars exactly
        // ---------------------------------------------------------------
        printf("\x1b");                                           // clear LCD
        printf("S1:%3d>%3dC %s\n", s1.raw, s1.weighted, st1);   // row 0
        printf("S2:%3d>%3dC %s\n", s2.raw, s2.weighted, st2);   // row 1

        // ---------------------------------------------------------------
        // Serial-only extended report
        // ---------------------------------------------------------------
        printf("\r==============================\n");
        printf("\r  Lab 4.2 – Signal Conditioning\n");
        printf("\r==============================\n");

        // --- Sensor 1 ---
        printf("\r [S1 - ANALOG (Potentiometer)]\n");
        printf("\r  ADC:       %4d counts\n",       adc1);
        printf("\r  Voltage:   %4d mV\n",           voltage1);
        printf("\r  Pipeline:\n");
        printf("\r    raw       = %4d C\n",         s1.raw);
        printf("\r    saturated = %4d C  [%d..%d C]\n",
               s1.saturated, SAT_LOW_1, SAT_HIGH_1);
        printf("\r    median    = %4d C  (window=%d)\n",
               s1.median, FILTER_WINDOW);
        printf("\r    weighted  = %4d C  (lin ramp 1..%d)\n",
               s1.weighted, FILTER_WINDOW);
        printf("\r  Thresholds: HI=%d C  LO=%d C\n",
               ALERT42_THRESHOLD_HIGH_1, ALERT42_THRESHOLD_LOW_1);
        printf("\r  Debounce:   %d / %d  Pending:%s\n",
               s1.bounce_count, ANTIBOUNCE42_SAMPLES_1,
               s1.pending_state ? "ALERT" : "OK");
        printf("\r  STATUS:     %s\n",
               s1.alert_active ? "!! ALERT !!" : "OK");
        printf("\r------------------------------\n");

        // --- Sensor 2 ---
        // Schimbă eticheta în funcție de senzor: DHT11 sau DHT22
        printf("\r [S2 - DIGITAL (DHT11)]\n");  // ← placă fizică; pentru Wokwi înlocuiește cu DHT22
        printf("\r  RAW(x0.1C):%4d\n",             raw_dht);
        printf("\r  Humidity:  %4d %%\n",           humidity);
        printf("\r  Pipeline:\n");
        printf("\r    raw       = %4d C\n",         s2.raw);
        printf("\r    saturated = %4d C  [%d..%d C]\n",
               s2.saturated, SAT_LOW_2, SAT_HIGH_2);
        printf("\r    median    = %4d C  (window=%d)\n",
               s2.median, FILTER_WINDOW);
        printf("\r    weighted  = %4d C  (lin ramp 1..%d)\n",
               s2.weighted, FILTER_WINDOW);
        printf("\r  Thresholds: HI=%d C  LO=%d C\n",
               ALERT42_THRESHOLD_HIGH_2, ALERT42_THRESHOLD_LOW_2);
        printf("\r  Debounce:   %d / %d  Pending:%s\n",
               s2.bounce_count, ANTIBOUNCE42_SAMPLES_2,
               s2.pending_state ? "ALERT" : "OK");
        printf("\r  STATUS:     %s\n",
               s2.alert_active ? "!! ALERT !!" : "OK");
        printf("\r==============================\n");

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
