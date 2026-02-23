#ifndef SRV_OS_SEQ_H
#define SRV_OS_SEQ_H

#define OS_TICK_MS   1
#define MS_PER_SEC   1000

#define SCHED_BTN_PERIOD     1
#define SCHED_BLINK_PERIOD   (100 / OS_TICK_MS)
#define SCHED_REPORT_PERIOD  (10 * MS_PER_SEC / OS_TICK_MS)

#define SCHED_BTN_OFFSET     0
#define SCHED_BLINK_OFFSET   (50  / OS_TICK_MS)
#define SCHED_REPORT_OFFSET  (200 / OS_TICK_MS)

extern int btn_sched_cnt;
extern int blink_sched_cnt;
extern int report_sched_cnt;

void svr_os_seq_setup();

#endif