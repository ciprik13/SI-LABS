#include "task_2.h"
#include "task_1.h"
#include "dd_led/dd_led.h"
#include <stdio.h>

volatile int  g_total_presses  = 0;
volatile int  g_short_presses  = 0;
volatile int  g_long_presses   = 0;
volatile long g_total_duration = 0;

void task_2(void *pvParameters)
{
    const TaskConfig_t *cfg = (const TaskConfig_t *)pvParameters;

    /* Offset initial */
    if (cfg->offset_ms > 0)
        vTaskDelay(pdMS_TO_TICKS(cfg->offset_ms));

    while (1)
    {
        /* Blocat până când Task 1 semnalizează o apăsare */
        if (xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdTRUE)
        {
            /* Citește durata ultimei apăsări (mutex-protejat) */
            int duration = 0;
            if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE)
            {
                duration = g_last_duration;
                xSemaphoreGive(xMutex);
            }

            int is_short = (duration < 500);
            int blinks   = is_short ? 5 : 10;

            /* Actualizează statisticile și citește local pentru printf
               — totul în același lock pentru a evita race condition */
            int total, shorts, longs;
            if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE)
            {
                g_total_presses++;
                g_total_duration += duration;
                if (is_short)
                    g_short_presses++;
                else
                    g_long_presses++;

                /* Copie locală — printf în afara mutex-ului e sigur */
                total  = g_total_presses;
                shorts = g_short_presses;
                longs  = g_long_presses;
                xSemaphoreGive(xMutex);
            }

            printf("\rTask2: total=%d short=%d long=%d dur=%dms\n",
                   total, shorts, longs, duration);

            /* Debochează Task1 imediat — poate accepta următoarea apăsare
               în timp ce Task2 execută animația de blink */
            xSemaphoreGive(xReady);

            /* Blink LED galben: 5x scurtă / 10x lungă */
            for (int i = 0; i < blinks; i++)
            {
                dd_led_2_turn_on();
                vTaskDelay(pdMS_TO_TICKS(100));
                dd_led_2_turn_off();
                vTaskDelay(pdMS_TO_TICKS(100));
            }
        }
    }
}