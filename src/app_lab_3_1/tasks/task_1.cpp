#include "task_1.h"
#include "dd_led/dd_led.h"
#include "dd_button/dd_button.h"

int g_last_press_duration_ms = 0;
int g_new_press_event = 0;

static int btn_was_pressed = 0;
static int press_tick = 0;
static int debounce_cnt = 0;

#define DEBOUNCE_MS 50
#define MIN_PRESS_MS 10

void task_1_setup() {
    btn_was_pressed = 0;
    press_tick = 0;
    debounce_cnt = 0;
    g_last_press_duration_ms = 0;
    g_new_press_event = 0;
}

void task_1_loop() {
    if (debounce_cnt > 0) {
        debounce_cnt--;
        return;
    }

    int btn_now = dd_button_is_pressed();

    if (btn_now && !btn_was_pressed) {
        press_tick = 0;
        btn_was_pressed = 1;
        debounce_cnt = DEBOUNCE_MS;
    }

    if (btn_now && btn_was_pressed) {
        press_tick++;
    }

    if (!btn_now && btn_was_pressed) {
        btn_was_pressed = 0;
        debounce_cnt = DEBOUNCE_MS;

        if (press_tick < MIN_PRESS_MS) {
            return;
        }

        g_last_press_duration_ms = press_tick;
        g_new_press_event = 1;

        if (press_tick < PRESS_THRESHOLD_MS) {
            dd_led_set_target(0);
            dd_led_1_set_target(1);
            printf("TASK 1: SHORT press %dms - GREEN LED ON\n", press_tick);
        } else {
            dd_led_1_set_target(0);
            dd_led_set_target(1);
            printf("TASK 1: LONG press %dms - RED LED ON\n", press_tick);
        }
    }
}