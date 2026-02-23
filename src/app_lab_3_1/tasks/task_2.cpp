#include "task_2.h"
#include "task_1.h"
#include "dd_led/dd_led.h"

int g_press_count    = 0;
int g_quick_count    = 0;
int g_hold_count     = 0;
int g_quick_total_ms = 0;
int g_hold_total_ms  = 0;

static int flashes_left  = 0;
static int led_state     = 0;

void task_2_setup() {
    g_press_count    = 0;
    g_quick_count    = 0;
    g_hold_count     = 0;
    g_quick_total_ms = 0;
    g_hold_total_ms  = 0;
    flashes_left     = 0;
    led_state        = 0;
}

void task_2_loop() {
    if (g_press_ready) {
        int duration = g_press_length_ms;
        g_press_ready = 0;
        g_press_count++;

        if (duration < BTN_LONG_THRESHOLD_MS) {
            g_quick_count++;
            g_quick_total_ms += duration;
            flashes_left = 5 * 2;
        } else {
            g_hold_count++;
            g_hold_total_ms += duration;
            flashes_left = 10 * 2;
        }

        led_state = 0;
        printf("\rTASK 2: total=%d short=%d long=%d\n",
               g_press_count, g_quick_count, g_hold_count);
    }

    if (flashes_left > 0) {
        led_state = !led_state;
        dd_led_2_set_target(led_state);
        flashes_left--;
        if (flashes_left == 0) {
            dd_led_2_set_target(0);
        }
    }
}