#include "task_1.h"
#include "dd_led/dd_led.h"
#include "dd_button/dd_button.h"

void task_1_setup(){

}

void task_1_loop(){
    if(dd_button_is_pressed()){
        printf("TASK 1: Button Pressed Detected\n");
        if (dd_led_is_on()){
            dd_led_turn_off();
    } else {
            dd_led_turn_on();
        }
        delay(300);
    }
}