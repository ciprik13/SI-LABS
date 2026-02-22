#ifndef SRV_OS_SEQ_H
#define SRV_OS_SEQ_H

#define SRV_OS_SYS_TICK 1
#define TIME_SEC 1000

// ---- Lab 3.1 timings ----
#define APP_LAB_3_1_TASK_1_REC    1
#define APP_LAB_3_1_TASK_2_REC    (100 / SRV_OS_SYS_TICK)
#define APP_LAB_3_1_TASK_3_REC    (10 * TIME_SEC / SRV_OS_SYS_TICK)

#define APP_LAB_3_1_TASK_1_OFFSET 0
#define APP_LAB_3_1_TASK_2_OFFSET (50  / SRV_OS_SYS_TICK)
#define APP_LAB_3_1_TASK_3_OFFSET (200 / SRV_OS_SYS_TICK)

extern int task_1_cnt;
extern int task_2_cnt;
extern int task_3_cnt;

void svr_os_seq_setup();

#endif