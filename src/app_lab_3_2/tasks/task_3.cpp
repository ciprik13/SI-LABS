#include "task_3.h"
#include "task_2.h"
#include <stdio.h>

void task_3(void *pvParameters)
{
    while (1)
    {
        int data;

        while (xQueueReceive(xQueue, &data, 200 / portTICK_PERIOD_MS) == pdTRUE)
        {
            if (data == 0)
            {
                printf("\nBuffer content: ");
            }

            printf("Task 3: Received data: %d, ", data);
        }

        printf("Task 3: Buffer Empty\n");
    }
}
