#include "srv_os_seq/srv_os_seq.h"
#include "app_lab_3_1/tasks/task_1.h"
#include "app_lab_3_1/tasks/task_2.h"
#include "app_lab_3_1/tasks/task_3.h"
#include "timer-api.h"

int task_1_cnt = SCHED_BTN_OFFSET    + SCHED_BTN_PERIOD;
int task_2_cnt = SCHED_BLINK_OFFSET  + SCHED_BLINK_PERIOD;
int task_3_cnt = SCHED_REPORT_OFFSET + SCHED_REPORT_PERIOD;

void svr_os_seq_setup() {
    timer_init_ISR_1KHz(TIMER_DEFAULT);
}

void timer_handle_interrupts(int timer) {
    if (--task_1_cnt <= 0) {
        task_1_cnt = SCHED_BTN_PERIOD;
        task_1_loop();
    }
    if (--task_2_cnt <= 0) {
        task_2_cnt = SCHED_BLINK_PERIOD;
        task_2_loop();
    }
    if (--task_3_cnt <= 0) {
        task_3_cnt = SCHED_REPORT_PERIOD;
        task_3_loop();
    }
}