#include "task_2.h"
#include "task_3.h"
#include "dd_led/dd_led.h"
#include "dd_button/dd_button.h"


void task_2_setup()
{

}

void task_2_loop()
{
    int blink_delay = g_task3_blink_count * 100;

    if (!dd_led_is_on())
    {
        if (!dd_led_1_is_on())
        {
            dd_led_1_turn_on();
            delay(blink_delay);
            dd_led_1_turn_off();
            delay(blink_delay);
        }
        else
        {
            dd_led_1_turn_off();
        }
    }
}