#ifndef APP_LAB_3_2_TASK_2_H
#define APP_LAB_3_2_TASK_2_H

#include "Arduino_FreeRTOS.h"
#include "task_config.h"

extern volatile int  g_total_presses;
extern volatile int  g_short_presses;
extern volatile int  g_long_presses;
extern volatile long g_total_duration;

void task_2(void *pvParameters);

#endif