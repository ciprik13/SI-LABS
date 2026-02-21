#include "srv_os_seq/srv_os_seq.h"
#include "app_lab_2_1/tasks/task_1.h"
#include "app_lab_2_1/tasks/task_2.h"
#include "app_lab_2_1/tasks/task_3.h"
#include <stdio.h>

#include "timer-api.h"

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

    // freq=1Hz, period=1s
    // timer_init_ISR_1Hz(TIMER_DEFAULT);

    // freq=2Hz, period=500ms
    // timer_init_ISR_2Hz(TIMER_DEFAULT);

    // freq=5Hz, period=200ms
    // частота=5Гц, период=200мс
    // timer_init_ISR_5Hz(TIMER_DEFAULT);

    // timer_init_ISR_500KHz(TIMER_DEFAULT);
    // timer_init_ISR_200KHz(TIMER_DEFAULT);
    // timer_init_ISR_100KHz(TIMER_DEFAULT);
    // timer_init_ISR_50KHz(TIMER_DEFAULT);
    // timer_init_ISR_20KHz(TIMER_DEFAULT);
    // timer_init_ISR_10KHz(TIMER_DEFAULT);
    // timer_init_ISR_5KHz(TIMER_DEFAULT);
    // timer_init_ISR_2KHz(TIMER_DEFAULT);
    timer_init_ISR_1KHz(TIMER_DEFAULT);
    // timer_init_ISR_500Hz(TIMER_DEFAULT);
    // timer_init_ISR_200Hz(TIMER_DEFAULT);
    // timer_init_ISR_100Hz(TIMER_DEFAULT);
    // timer_init_ISR_50Hz(TIMER_DEFAULT);
    // timer_init_ISR_20Hz(TIMER_DEFAULT);
    // timer_init_ISR_10Hz(TIMER_DEFAULT);
    // timer_init_ISR_5Hz(TIMER_DEFAULT);
    // timer_init_ISR_2Hz(TIMER_DEFAULT);
    // timer_init_ISR_1Hz(TIMER_DEFAULT);
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