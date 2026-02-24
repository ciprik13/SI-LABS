#ifndef APP_LAB_3_2_TASK_2_H
#define APP_LAB_3_2_TASK_2_H

#include "Arduino_FreeRTOS.h"
#include "queue.h"

extern QueueHandle_t xQueue;

void task_2(void *pvParameters);

#endif
