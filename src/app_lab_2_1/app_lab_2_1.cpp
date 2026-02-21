#include "app_lab_2_1.h"
#include "srv_serial_stdio/srv_serial_stdio.h"
#include "tasks/task_1.h"
#include "tasks/task_2.h"
#include "tasks/task_3.h"
#include "dd_led/dd_led.h"
#include "dd_button/dd_button.h"
#include "srv_os_seq/srv_os_seq.h"


void app_lab_2_1_setup() {
    dd_led_setup();
    dd_button_setup();
    srv_serial_stdio_setup();
    svr_os_seq_setup();
    task_1_setup();
    task_2_setup();
    task_3_setup();
    printf("App Lab 2.1: Started\n");
}

void app_lab_2_1_loop() {
    dd_led_apply();

    printf("App Lab 2.1: Idle | LED1=%s | LED2(blink)=%s | BlinkCount=%d\n",
           dd_led_is_on()   ? "ON" : "OFF",
           dd_led_1_is_on() ? "ON" : "OFF",
           g_task3_blink_count);
    delay(1000);
}