#include "task_2.h"
#include "dd_led/dd_led.h"
#include "task_3.h"

static int blink_tick_cnt = 0;

void app_lab_2_1_task_2_setup()
{
    blink_tick_cnt = 0;
}

void app_lab_2_1_task_2_loop()
{
    if (dd_led_is_on())
    {
        blink_tick_cnt = 0;
        dd_led_1_set_target(0);
        return;
    }
    blink_tick_cnt++;
    if (blink_tick_cnt >= g_task3_blink_count)
    {
        blink_tick_cnt = 0;
        dd_led_1_set_target(!dd_led_1_is_on());
        printf("TASK 2: LED2=%s (BlinkCount=%d)\n",
               dd_led_1_is_on() ? "ON" : "OFF", g_task3_blink_count);
    }
}