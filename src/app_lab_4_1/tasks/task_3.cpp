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
// LCD row 0: S1 (analog potentiometer) temperature + alert status
// LCD row 1: S2 (digital DHT22)        temperature + alert status
// Serial: full structured report for both sensors via printf.
// ===========================================================================
void task_report(void *pvParameters) {
    (void) pvParameters;

    for (;;) {
        // --- Snapshot S1 (analog) -------------------------------------------
        int temp1    = dd_sns_temperature_get_celsius();
        int raw1     = dd_sns_temperature_get_raw();
        int voltage1 = dd_sns_temperature_get_voltage();

        CondState_t snap1 = { false, false, 0 };
        if (xSemaphoreTake(g_cond_mutex, portMAX_DELAY) == pdTRUE) {
            snap1 = g_cond;
            xSemaphoreGive(g_cond_mutex);
        }

        // --- Snapshot S2 (digital DHT22) ------------------------------------
        int temp2    = dd_sns_dht_get_celsius();
        int raw2     = dd_sns_dht_get_raw();       // temp × 10
        int humidity = dd_sns_dht_get_humidity();

        CondState_t snap2 = { false, false, 0 };
        if (xSemaphoreTake(g_cond2_mutex, portMAX_DELAY) == pdTRUE) {
            snap2 = g_cond2;
            xSemaphoreGive(g_cond2_mutex);
        }

        const char *st1 = snap1.alert_active ? "[ALT]" : snap1.pending_state ? "[PND]" : " [OK]";
        const char *st2 = snap2.alert_active ? "[ALT]" : snap2.pending_state ? "[PND]" : " [OK]";

        // --- LCD (rows 0-1) + Serial (all rows) via tee'd stdout ------------
        printf("\x1b");                                    // clear LCD
        printf("S1:%3dC %s\n", temp1, st1);                // LCD row 0
        printf("S2:%3dC %s\n", temp2, st2);                // LCD row 1
        // Serial-only from here ----------------------------------------------
        printf("\r==============================\n");
        printf("\r [S1 - ANALOG (Potentiometer)]\n");
        printf("\r  RAW:     %4d ADC\n",  raw1);
        printf("\r  Voltage: %4d mV\n",   voltage1);
        printf("\r  Temp:    %4d C\n",    temp1);
        printf("\r  Thr HI:  %d C  LO: %d C\n", ALERT_THRESHOLD_HIGH, ALERT_THRESHOLD_LOW);
        printf("\r  Bounce:  %d / %d  Pending: %s\n",
               snap1.bounce_count, ANTIBOUNCE_SAMPLES,
               snap1.pending_state ? "ALERT" : "OK");
        printf("\r  STATUS:  %s\n", snap1.alert_active ? "!! ALERT !!" : "OK");
        printf("\r------------------------------\n");
        // For DHT11 on physical board, change label: printf("\r [S2 - DIGITAL (DHT11)]\n");
        printf("\r [S2 - DIGITAL (DHT11)]\n");
        printf("\r  RAW:     %4d (x0.1 C)\n", raw2);
        printf("\r  Temp:    %4d C\n",        temp2);
        printf("\r  Humidity:%4d %%\n",        humidity);
        printf("\r  Thr HI:  %d C  LO: %d C\n", ALERT2_THRESHOLD_HIGH, ALERT2_THRESHOLD_LOW);
        printf("\r  Bounce:  %d / %d  Pending: %s\n",
               snap2.bounce_count, ANTIBOUNCE2_SAMPLES,
               snap2.pending_state ? "ALERT" : "OK");
        printf("\r  STATUS:  %s\n", snap2.alert_active ? "!! ALERT !!" : "OK");
        printf("\r==============================\n");

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}