#include "app_lab_1_1.h"

#include "Arduino.h"
#include <string.h>

#include "srv_serial_stdio/srv_serial_stdio.h"
#include "dd_led/dd_led.h"

void app_lab_1_1_setup() {
    srv_serial_stdio_setup();
    dd_led_setup();
    
    printf("app_lab_1_1: Started\n");
    printf("Type 'help' for a list of commands.\n");
}

char cmd[32];

void app_lab_1_1_loop() {
    printf("\r\nEnter command: ");
    
    scanf(" %31[^\r\n]", cmd);  
    
    printf("\r\nCommand: %s\n", cmd);

    if (strcmp(cmd, "led on") == 0){
        printf("\rLED ON\n");
        dd_led_turn_on(LED_RED);
    }
    else if (strcmp(cmd, "led off") == 0){
        printf("\rLED OFF\n");
        dd_led_turn_off(LED_RED);
    }
    else if (strcmp(cmd, "help") == 0) {
        printf("\rAvailable Commands:\n");
        printf(" - led on    : Turns the LED permanently ON\n");
        printf(" - led off   : Turns the LED permanently OFF\n");
        printf(" - led blink : Blinks the LED 3 times\n");
        printf(" - help      : Displays this menu\n");
    }
    else if (strcmp(cmd, "led blink") == 0) {
        printf("\rBlinking LED Sequence...\n");
        for(int i = 0; i < 3; i++) {
            dd_led_turn_on(LED_RED);
            delay(200); 
            dd_led_turn_off(LED_RED);
            delay(200); 
        }
        printf("Sequence Complete.\n");
    }
    else {
        printf("\rError: Unknown command '%s'. Type 'help'.\n", cmd);
    }

    delay(1000);
}
