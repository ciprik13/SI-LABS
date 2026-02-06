#include "app_lab_1_1.h"

#include "Arduino.h"
#include <string.h>

#include "srv_serial_stdio/srv_serial_stdio.h"
#include "dd_led/dd_led.h"

void app_lab_1_1_setup(){
    srv_serial_stdio_setup();
    dd_led_setup();
    printf("app_lab_1_1: Started\n");
}

char cmd[32];
void app_lab_1_1_loop(){
    printf("\r\nEnter command: led on/off: ");
    scanf(" %[^\n]", cmd);

    printf("\r\nCommand: %s\n", cmd);

    if (strcmp(cmd, "led on") == 0){
            printf("LED is ON\n");
            dd_led_turn_on();
        }
    else if (strcmp(cmd, "led off") == 0){
            printf("LED is OFF\n");
            dd_led_turn_off();
        }
    else {
        printf("\rError: Unknown command. Try led on/off\n");
    }

    delay(1000);
}