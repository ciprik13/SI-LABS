#ifndef APP_LAB_3_2_TASK_1_H
#define APP_LAB_3_2_TASK_1_H

#include "Arduino_FreeRTOS.h"
#include "semphr.h"

#define ONE_SEC       1000
#define TASK_1_REC    10
#define TASK_1_OFFSET 100

extern SemaphoreHandle_t xSemaphore;

void task_1(void *pvParameters);

#endif
