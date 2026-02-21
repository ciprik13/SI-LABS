#include "srv_os_seq/srv_os_seq.h"
#include "app_lab_2_1/tasks/task_1.h"
#include "app_lab_2_1/tasks/task_2.h"
#include "app_lab_2_1/tasks/task_3.h"
#include <stdio.h>

#include "timer-api.h"

int task_1_cnt = APP_LAB_2_1_TASK_1_OFFSET + APP_LAB_2_1_TASK_1_REC;
int task_2_cnt = APP_LAB_2_1_TASK_2_OFFSET + APP_LAB_2_1_TASK_2_REC;
int task_3_cnt = APP_LAB_2_1_TASK_3_OFFSET + APP_LAB_2_1_TASK_3_REC;

void svr_os_seq_setup()
{
    timer_init_ISR_1KHz(TIMER_DEFAULT);
}

void timer_handle_interrupts(int timer)
{
    if (--task_1_cnt <= 0)
    {
        task_1_cnt = APP_LAB_2_1_TASK_1_REC;
        task_1_loop();
    }

    if (--task_2_cnt <= 0)
    {
        task_2_cnt = APP_LAB_2_1_TASK_2_REC;
        task_2_loop();
    }

    if (--task_3_cnt <= 0)
    {
        task_3_cnt = APP_LAB_2_1_TASK_3_REC;
        task_3_loop();
    }
}