#ifndef APP_LAB_2_1_TASK_3_H
#define APP_LAB_2_1_TASK_3_H

#define TASK_3_VAR_MIN 1
#define TASK_3_VAR_MAX 10
#define TASK_3_VAR_DEFAULT 5

extern volatile int g_task3_blink_count;

void task_3_setup();
void task_3_loop();

#endif