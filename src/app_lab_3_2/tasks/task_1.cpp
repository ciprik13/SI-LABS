#include "task_1.h"
#include "dd_button/dd_button.h"
#include "dd_led/dd_led.h"
#include <Arduino.h>
#include <stdio.h>

SemaphoreHandle_t xSemaphore     = NULL;
SemaphoreHandle_t xMutex         = NULL;
SemaphoreHandle_t xReady         = NULL;
volatile int      g_last_duration = 0;

void task_1(void *pvParameters)
{
    const TaskConfig_t *cfg = (const TaskConfig_t *)pvParameters;

    /* Offset initial — decalaj față de celelalte task-uri */
    if (cfg->offset_ms > 0)
        vTaskDelay(pdMS_TO_TICKS(cfg->offset_ms));

    TickType_t    xLastWakeTime = xTaskGetTickCount();
    int           was_pressed   = 0;
    unsigned long press_start   = 0;
    int           led_timer     = 0;

    /* Numărul de perioade cât rămâne aprins LED-ul (LED_ON_MS / period_ms) */
    const int led_on_ticks = LED_ON_MS / cfg->period_ms;

    while (1)
    {
        int is_pressed = dd_button_is_pressed();

        /* --- Tranziție: buton tocmai apăsat (rising edge) --- */
        if (is_pressed && !was_pressed)
        {
            press_start = millis();
        }
        /* --- Tranziție: buton tocmai eliberat (falling edge) --- */
        else if (!is_pressed && was_pressed)
        {
            int duration_ms = (int)(millis() - press_start);

            /* Salvează durata în variabila globală (protejat cu mutex) */
            if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE)
            {
                g_last_duration = duration_ms;
                xSemaphoreGive(xMutex);
            }

            /* Ignore sub-debounce glitches caused by signal bounce */
            if (duration_ms < DEBOUNCE_MIN_MS)
            {
                /* spurious bounce — reset and keep waiting */
                press_start = millis();
            }
            else
            {
                /* Semnalizare vizuală: verde = scurtă, roșu = lungă */
                if (duration_ms < SHORT_PRESS_MS)
                {
                    dd_led_1_turn_on();   /* GREEN */
                    dd_led_turn_off();    /* RED   */
                }
                else
                {
                    dd_led_turn_on();     /* RED   */
                    dd_led_1_turn_off();  /* GREEN */
                }
                led_timer = led_on_ticks;

                /* Semnalizează Task 2 doar dacă nu e ocupat (alternare strictă) */
                if (xSemaphoreTake(xReady, 0) == pdTRUE)
                {
                    xSemaphoreGive(xSemaphore);

                    printf("\rTask1: press %d ms (%s)\n",
                           duration_ms,
                           duration_ms < SHORT_PRESS_MS ? "SHORT" : "LONG");
                }
            }
        }

        was_pressed = is_pressed;

        /* Stinge LED-ul de indicație după expirarea timpului */
        if (led_timer > 0)
        {
            led_timer--;
            if (led_timer == 0)
            {
                dd_led_turn_off();
                dd_led_1_turn_off();
            }
        }

        /* Recurență precisă: polling la fiecare cfg->period_ms ms */
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(cfg->period_ms));
    }
}