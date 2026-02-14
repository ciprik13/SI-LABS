#ifndef DD_LED_H
#define DD_LED_H

#include <Arduino.h>

#define LED_RED 0
#define LED_GREEN 1

#define LED_RED_PIN 10
#define LED_GREEN_PIN 11

void dd_led_setup();
void dd_led_turn_on(int led_id);
void dd_led_turn_off(int led_id);

int dd_led_is_on(int led_id);


#endif