#include "task_3.h"
#include "task_2.h"
#include "srv_serial_stdio/srv_serial_stdio.h"

void task_3_setup() {
    // nothing to initialize
}

void task_3_loop() {
    int total  = g_total_presses;
    int shorts = g_short_presses;
    int longs  = g_long_presses;
    int avg_ms = 0;

    if (total > 0) {
        avg_ms = (g_sum_short_ms + g_sum_long_ms) / total;
    }

    printf("=== TASK 3 REPORT (10s) ===\n");
    printf("  Total presses : %d\n",   total);
    printf("  Short (<500ms): %d\n",   shorts);
    printf("  Long (>=500ms): %d\n",   longs);
    printf("  Avg duration  : %d ms\n", avg_ms);
    printf("==========================\n");

    // Reset statistics
    g_total_presses = 0;
    g_short_presses = 0;
    g_long_presses  = 0;
    g_sum_short_ms  = 0;
    g_sum_long_ms   = 0;
}