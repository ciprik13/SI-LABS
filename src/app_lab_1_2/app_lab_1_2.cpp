#include "app_lab_1_2.h"
#include "srv_stdio_lcd/srv_stdio_lcd.h"
#include "srv_stdio_keypad/srv_stdio_keypad.h"
#include <stdio.h>
#include <Arduino.h>

extern LiquidCrystal_I2C lcd;

static const uint8_t app_led_red_pin = 10;
static const uint8_t app_led_green_pin = 11;

void app_lab_1_2_setup() {
    srv_stdio_lcd_setup();
    delay(500);
    srv_stdio_keypad_setup();
    delay(500);
    pinMode(app_led_red_pin, OUTPUT);
    pinMode(app_led_green_pin, OUTPUT);
    digitalWrite(app_led_red_pin, LOW);
    digitalWrite(app_led_green_pin, LOW);
    delay(500);
    
    lcd.clear();
    printf("System Ready!");
    delay(2000);
}

char code[4] = {'1', '2', '3', '4'};
char input[4] = {0, 0, 0, 0};

void app_lab_1_2_loop() {
    
    input[0] = 0;
    input[1] = 0;
    input[2] = 0;
    input[3] = 0;
    
    lcd.clear();
    printf("Enter Code:");
    lcd.setCursor(0, 1);
    
    for(int i = 0; i < 4; i++) {
        scanf("%c", &input[i]);
        printf("%c", input[i]);
        delay(50);
    }

    delay(500);
    lcd.clear();

    if (input[0] == code[0] && input[1] == code[1] && input[2] == code[2] && input[3] == code[3]) {
        printf("Correct code!");
        digitalWrite(app_led_green_pin, HIGH);
        digitalWrite(app_led_red_pin, LOW);
    } else {
        printf("Wrong code!");
        digitalWrite(app_led_red_pin, HIGH);
        digitalWrite(app_led_green_pin, LOW);
    }

    delay(2000);
    digitalWrite(app_led_green_pin, LOW);
    digitalWrite(app_led_red_pin, LOW);
}
