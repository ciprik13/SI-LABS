#ifndef DD_LED_H
#define DD_LED_H

#include <Arduino.h>

#define LED_PIN   13
#define LED_1_PIN 12

void dd_led_setup();

void dd_led_set_target(int val);
void dd_led_1_set_target(int val);

void dd_led_apply();

int dd_led_is_on();
int dd_led_1_is_on();

void dd_led_turn_on();
void dd_led_turn_off();
void dd_led_1_turn_on();
void dd_led_1_turn_off();

#endif