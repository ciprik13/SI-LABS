#ifndef APP_LAB_3_2_TASK_1_H
#define APP_LAB_3_2_TASK_1_H

#include "Arduino_FreeRTOS.h"
#include "semphr.h"
#include "task_config.h"

#define DEBOUNCE_MIN_MS 50    // ignored if shorter (bounce filter)
#define SHORT_PRESS_MS  500   // pragul scurt/lung (ms)
#define LED_ON_MS       10000  // cât timp rămâne aprins LED-ul de indicație (ms)

extern SemaphoreHandle_t xSemaphore;  // semnalizare Task1 → Task2
extern SemaphoreHandle_t xMutex;      // protecție variabile partajate
extern SemaphoreHandle_t xReady;      // alternare strictă T1→T2→T1→T2
extern volatile int g_last_duration;  // ultima durată măsurată (ms)

void task_1(void *pvParameters);

#endif