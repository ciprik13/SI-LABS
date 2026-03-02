#include "task_3.h"
#include <stdio.h>

void task_3(void *pvParameters)
{
    const TaskConfig_t *cfg = (const TaskConfig_t *)pvParameters;

    /* Offset initial */
    if (cfg->offset_ms > 0)
        vTaskDelay(pdMS_TO_TICKS(cfg->offset_ms));

    /* Referință de timp pentru vTaskDelayUntil — recurență precisă */
    TickType_t xLastWakeTime = xTaskGetTickCount();

    while (1)
    {
        /* Așteaptă exact cfg->period_ms ms față de ultima trezire */
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(cfg->period_ms));

        int  total, shorts, longs;
        long total_dur;

        /* Snapshot + resetare statistici — atomic sub mutex */
        if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE)
        {
            total     = g_total_presses;
            shorts    = g_short_presses;
            longs     = g_long_presses;
            total_dur = g_total_duration;

            g_total_presses  = 0;
            g_short_presses  = 0;
            g_long_presses   = 0;
            g_total_duration = 0;

            xSemaphoreGive(xMutex);
        }

        int avg_ms = (total > 0) ? (int)(total_dur / total) : 0;

        printf("\r=== Task 3: Periodic Report (last %u s) ===\n",
               cfg->period_ms / 1000);
        printf("\rTotal presses  : %d\n", total);
        printf("\rShort (<500ms) : %d\n", shorts);
        printf("\rLong  (>=500ms): %d\n", longs);
        printf("\rAverage duration: %d ms\n", avg_ms);
        printf("\r==========================================\n\n");
    }
}