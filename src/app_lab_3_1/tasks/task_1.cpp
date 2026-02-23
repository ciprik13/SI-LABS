#include "task_1.h"
#include "dd_button/dd_button.h"
#include "dd_led/dd_led.h"

int g_press_length_ms = 0;
int g_press_ready     = 0;

static int btn_held       = 0;
static int hold_counter   = 0;
static int dbnc_timer     = 0;

#define DBNC_DELAY_MS   50
#define VALID_PRESS_MS  10

void task_1_setup() {
    g_press_length_ms = 0;
    g_press_ready     = 0;
    btn_held          = 0;
    hold_counter      = 0;
    dbnc_timer        = 0;
}

void task_1_loop() {
    if (dbnc_timer > 0) {
        dbnc_timer--;
        return;
    }

    int reading = dd_button_is_pressed();

    if (reading && btn_held) {
        hold_counter++;
    }

    if (reading && !btn_held) {
        btn_held      = 1;
        hold_counter  = 0;
        dbnc_timer    = DBNC_DELAY_MS;
    }

    if (!reading && btn_held) {
        btn_held   = 0;
        dbnc_timer = DBNC_DELAY_MS;

        if (hold_counter < VALID_PRESS_MS) {
            return;
        }

        g_press_length_ms = hold_counter;
        g_press_ready     = 1;

        if (hold_counter < BTN_LONG_THRESHOLD_MS) {
            dd_led_1_set_target(1);
            dd_led_set_target(0);
            printf("\rTASK 1: SHORT press %dms - GREEN LED ON\n", hold_counter);
        } else {
            dd_led_set_target(1);
            dd_led_1_set_target(0);
            printf("\rTASK 1: LONG press %dms - RED LED ON\n", hold_counter);
        }
    }
}