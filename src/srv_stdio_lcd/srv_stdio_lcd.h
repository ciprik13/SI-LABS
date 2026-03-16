#ifndef SRV_STDIO_LCD_H
#define SRV_STDIO_LCD_H

#include <LiquidCrystal_I2C.h>

extern LiquidCrystal_I2C lcd;

void srv_stdio_lcd_setup();

#endif // SRV_STDIO_LCD_H