#include "task_2.h"
#include "task_1.h"
#include "dd_led/dd_led.h"

int g_total_presses = 0;
int g_short_presses = 0;
int g_long_presses  = 0;
int g_sum_short_ms  = 0;
int g_sum_long_ms   = 0;

static int blink_remaining = 0;
static int blink_phase = 0;

void task_2_setup() {
    g_total_presses = 0;
    g_short_presses = 0;
    g_long_presses  = 0;
    g_sum_short_ms  = 0;
    g_sum_long_ms   = 0;
    blink_remaining = 0;
    blink_phase     = 0;
}

void task_2_loop() {
    if (g_new_press_event) {
        g_new_press_event = 0;
        int dur = g_last_press_duration_ms;
        g_total_presses++;

        if (dur < PRESS_THRESHOLD_MS) {
            g_short_presses++;
            g_sum_short_ms += dur;
            blink_remaining = 5 * 2;
        } else {
            g_long_presses++;
            g_sum_long_ms += dur;
            blink_remaining = 10 * 2;
        }
        blink_phase = 0;
        printf("TASK 2: total=%d short=%d long=%d\n",
               g_total_presses, g_short_presses, g_long_presses);
    }

    if (blink_remaining > 0) {
        blink_phase = !blink_phase;
        dd_led_2_set_target(blink_phase);
        blink_remaining--;
        if (blink_remaining == 0) {
            dd_led_2_set_target(0);
        }
    }
}