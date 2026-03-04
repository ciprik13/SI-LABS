#ifndef SRV_STDIO_LCD_H
#define SRV_STDIO_LCD_H

#include <LiquidCrystal_I2C.h>
#include <stdio.h>

extern LiquidCrystal_I2C lcd;

void    srv_stdio_lcd_setup();
FILE   *srv_stdio_lcd_get_stream();  // returns the LCD FILE* for use with fprintf

#endif