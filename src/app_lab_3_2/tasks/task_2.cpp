#include "task_2.h"
#include "task_1.h"
#include "dd_led/dd_led.h"
#include <stdio.h>

QueueHandle_t xQueue = NULL;

static int task_N_cnt = 0;

void task_2(void *pvParameters)
{
    while (1)
    {
        if (xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdTRUE)
        {
            printf("Semaphore Detected\n");
            task_N_cnt++;

            for (int i = 0; i < task_N_cnt; i++)
            {
                xQueueSendToFront(xQueue, &i, portMAX_DELAY);
                vTaskDelay(50 / portTICK_PERIOD_MS);
            }
            printf("\nTask 2: Queue Sent\n");

            for (int i = 0; i < task_N_cnt; i++)
            {
                dd_led_turn_on();
                vTaskDelay(300 / portTICK_PERIOD_MS);
                dd_led_turn_off();
                vTaskDelay(500 / portTICK_PERIOD_MS);
                printf("LED Blinks\n");
            }
            printf("Task 2: LED Blinks done\n");
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
        printf("Task 2: Running\n");
    }
}
