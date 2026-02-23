#include "app_lab_3_1.h"
#include "dd_led/dd_led.h"
#include "dd_button/dd_button.h"
#include "srv_serial_stdio/srv_serial_stdio.h"
#include "srv_os_seq/srv_os_seq.h"
#include "tasks/task_1.h"
#include "tasks/task_2.h"
#include "tasks/task_3.h"

void app_lab_3_1_setup() {
    srv_serial_stdio_setup();
    dd_button_setup();
    dd_led_setup();
    task_1_setup();
    task_2_setup();
    task_3_setup();
    svr_os_seq_setup();
    printf("App Lab 3.1: Started\n");
}

void app_lab_3_1_loop() {
    dd_led_apply();
}