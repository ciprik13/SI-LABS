#include "task_3.h"
#include "dd_button/dd_button.h"

volatile int g_task3_blink_count = TASK_3_VAR_DEFAULT;

void task_3_setup() {

}

void task_3_loop() {
    if (dd_button_1_is_pressed()) {
        printf("TASK 3: Button UP Pressed\\n");
        if (g_task3_blink_count < TASK_3_VAR_MAX) {
            g_task3_blink_count++;
        }
        delay(300);
    }

    if (dd_button_2_is_pressed()) {
        printf("TASK 3: Button DOWN Pressed\\n");
        if (g_task3_blink_count > TASK_3_VAR_MIN) {
            g_task3_blink_count--;
        }
        delay(300);
    }
}