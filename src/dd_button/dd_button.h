#ifndef DD_BUTTON_H
#define DD_BUTTON_H
#include <Arduino.h>

#define BUTTON_PIN   2
#define BUTTON_1_PIN 3
#define BUTTON_2_PIN 4

void dd_button_setup();
int  dd_button_is_pressed();
int  dd_button_1_is_pressed();
int  dd_button_2_is_pressed();

#endif