#include "srv_stdio_lcd.h"
#include <Arduino.h>
#include <stdio.h>
#include <LiquidCrystal_I2C.h>

#define CLEAR_KEY 0x1b

int lcdColumns = 16;
int lcdRows = 2;

LiquidCrystal_I2C lcd(0x3F, lcdColumns, lcdRows);

int srv_stdio_lcd_put_char(char c, FILE *stream) {
    if (c == CLEAR_KEY){
        lcd.clear();
        lcd.setCursor(0, 0);
    } else {
        lcd.print(c);
    }
    return 0;
}

void srv_stdio_lcd_setup() {
    lcd.init();
    lcd.backlight();
    lcd.clear();
    lcd.home();
    FILE *srv_stdio_lcd_stream = fdevopen(srv_stdio_lcd_put_char, NULL);
    stdout = srv_stdio_lcd_stream;
} 