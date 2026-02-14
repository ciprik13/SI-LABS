
#include "app_lab_1_2.h"
#include "srv_stdio_lcd/srv_stdio_lcd.h"
#include "srv_stdio_keypad/srv_stdio_keypad.h"
#include "dd_led/dd_led.h"
#include <stdio.h>
#include <Arduino.h>

extern LiquidCrystal_I2C lcd;

void app_lab_1_2_setup() {
    srv_stdio_lcd_setup();
    delay(500);
    srv_stdio_keypad_setup();
    delay(500);
    dd_led_setup();
    delay(500);
    
    lcd.clear();
    lcd.print("System Ready!");
    delay(2000);
}

char code[4] = {'1', '2', '3', '4'};
char input[4] = {0, 0, 0, 0};

void app_lab_1_2_loop() {
    // Reset input buffer
    input[0] = 0;
    input[1] = 0;
    input[2] = 0;
    input[3] = 0;
    
    lcd.clear();
    lcd.print("Enter Code:");
    lcd.setCursor(0, 1);
    
    for(int i = 0; i < 4; i++) {
        scanf("%c", &input[i]);
        lcd.print(input[i]);
        delay(50);
    }

    delay(500);
    lcd.clear();

    if (input[0] == code[0] && input[1] == code[1] && input[2] == code[2] && input[3] == code[3]) {
        lcd.print("Correct code!");
        dd_led_turn_on(LED_GREEN);
        dd_led_turn_off(LED_RED);
    } else {
        lcd.print("Wrong code!");
        dd_led_turn_on(LED_RED);
        dd_led_turn_off(LED_GREEN);
    }

    delay(2000);
    dd_led_turn_off(LED_GREEN);
    dd_led_turn_off(LED_RED);
}
