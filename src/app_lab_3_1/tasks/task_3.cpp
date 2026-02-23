#include "task_3.h"
#include "task_2.h"
#include "srv_serial_stdio/srv_serial_stdio.h"

void task_3_setup() {
    
}

void task_3_loop() {
    int avg_duration_ms = 0;
    int total  = g_press_count;
    int shorts = g_quick_count;
    int longs  = g_hold_count;

    if (total > 0) {
        avg_duration_ms = (g_quick_total_ms + g_hold_total_ms) / total;
    }

    printf("\r TASK 3 REPORT (10s)\n");
    printf("\r  Total presses : %d\n",    total);
    printf("\r  Short (<500ms): %d\n",    shorts);
    printf("\r  Long (>=500ms): %d\n",    longs);
    printf("\r  Avg duration  : %d ms\n", avg_duration_ms);

    g_press_count    = 0;
    g_quick_count    = 0;
    g_hold_count     = 0;
    g_quick_total_ms = 0;
    g_hold_total_ms  = 0;
}