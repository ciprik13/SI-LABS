#include "app_lab_1_1.h"

#include "Arduino.h"

#include "srv_serial_stdio/srv_serial_stdio.h"
#include "dd_led/dd_led.h"

void app_lab_1_1_setup(){
    srv_serial_stdio_setup();
    dd_led_setup();
}

char cmd;
void app_lab_1_1_loop(){
    printf("Enter command: Led on / Led off\n");
    scanf(" %c", &cmd);

    if (cmd == '1')
        {
            printf("LED is ON\n");
            dd_led_turn_on();
        }
    else if (cmd == '0')
        {
            printf("LED is OFF\n");
            dd_led_turn_off();
        }

    printf("Command entered: %c\n", cmd);

    delay(1000);
}