#include "srv_stdio_lcd.h"
#include <Arduino.h>
#include <stdio.h>
#include <LiquidCrystal_I2C.h>

#define CLEAR_KEY 0x1b

int lcdColumns = 16;
int lcdRows    = 2;

LiquidCrystal_I2C lcd(0x3F, lcdColumns, lcdRows);

static int s_lcd_row = 0;  // tracks current LCD cursor row

int srv_stdio_lcd_put_char(char c, FILE *stream) {
    if (c == CLEAR_KEY) {
        lcd.clear();
        lcd.setCursor(0, 0);
        s_lcd_row = 0;
    } else if (c == '\n') {
        s_lcd_row++;
        if (s_lcd_row < lcdRows) {
            lcd.setCursor(0, s_lcd_row);
        }
        Serial.write('\r');
        Serial.write('\n');
    } else if (c == '\r') {
        Serial.write('\r');          // \r: Serial only, skip LCD
    } else {
        if (s_lcd_row < lcdRows) {   // rows 0-1 visible on LCD
            lcd.print(c);
        }
        Serial.write(c);             // all chars tee'd to Serial
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