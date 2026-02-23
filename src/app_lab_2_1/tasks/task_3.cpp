#include "task_3.h"
#include "dd_button/dd_button.h"

int g_task3_blink_count = TASK_3_VAR_DEFAULT;

static int debounce_cnt = 0;

void app_lab_2_1_task_3_setup() {
    g_task3_blink_count = TASK_3_VAR_DEFAULT;
    debounce_cnt = 0;
}

void app_lab_2_1_task_3_loop() {
    if (debounce_cnt > 0) {
        debounce_cnt--;
        return;
    }
    if (dd_button_1_is_pressed()) {
        if (g_task3_blink_count < TASK_3_VAR_MAX) {
            g_task3_blink_count++;
        }
        printf("TASK 3: Button UP - BlinkCount=%d\n", g_task3_blink_count);
        debounce_cnt = 3;
    }
    else if (dd_button_2_is_pressed()) {
        if (g_task3_blink_count > TASK_3_VAR_MIN) {
            g_task3_blink_count--;
        }
        printf("TASK 3: Button DOWN - BlinkCount=%d\n", g_task3_blink_count);
        debounce_cnt = 3;
    }
}