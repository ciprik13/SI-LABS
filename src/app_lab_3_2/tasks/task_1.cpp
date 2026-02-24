#include "task_1.h"
#include "dd_button/dd_button.h"
#include "dd_led/dd_led.h"
#include <stdio.h>

SemaphoreHandle_t xSemaphore = NULL;

static int task_1_led_time_cnt = -1;

void task_1(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    vTaskDelay(TASK_1_OFFSET / portTICK_PERIOD_MS);

    while (1)
    {
        printf("Task 1 is running\n");

        if (dd_button_is_pressed() && task_1_led_time_cnt < 0)
        {
            task_1_led_time_cnt = ONE_SEC / TASK_1_REC;
            dd_led_turn_on();
            printf("Button Pressed Detected\n");
        }
        if (task_1_led_time_cnt > 0)
        {
            task_1_led_time_cnt--;
            dd_led_turn_on();
            printf("Led ON\n");
        }
        else if (task_1_led_time_cnt == 0)
        {
            dd_led_turn_off();
            printf("Led OFF\n");
            xSemaphoreGive(xSemaphore);
            printf("Semaphore Given\n");
            task_1_led_time_cnt--;
        }

        vTaskDelayUntil(&xLastWakeTime, TASK_1_REC / portTICK_PERIOD_MS);
    }
}
