#include "task_1.h"
#include "dd_led/dd_led.h"
#include "dd_button/dd_button.h"

static int debounce_cnt = 0;

void task_1_setup(){
    debounce_cnt = 0;
}

void task_1_loop(){
    if (debounce_cnt > 0) {
        debounce_cnt--;
        return;
    }
    if (dd_button_is_pressed()) {
        if (dd_led_is_on()) {
            dd_led_set_target(0);
        } else {
            dd_led_set_target(1);
        }
        printf("TASK 1: Button Pressed - LED1=%s\n", dd_led_is_on() ? "ON" : "OFF");
        debounce_cnt = 3;
    }
}