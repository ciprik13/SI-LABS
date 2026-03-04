#include "srv_stdio_lcd.h"
#include <Arduino.h>
#include <stdio.h>
#include <LiquidCrystal_I2C.h>

#define CLEAR_KEY 0x1b

int lcdColumns = 16;
int lcdRows = 2;

LiquidCrystal_I2C lcd(0x3F, lcdColumns, lcdRows);

static FILE *s_lcd_stream = NULL;

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
    s_lcd_stream = fdevopen(srv_stdio_lcd_put_char, NULL);
    // Do NOT redirect stdout here – stdout stays on Serial so the terminal works.
}

FILE *srv_stdio_lcd_get_stream() {
    return s_lcd_stream;
}