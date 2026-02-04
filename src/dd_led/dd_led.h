#ifndef DD_LED_H_
#define DD_LED_H_

#include "Arduino.h"

#define LED_PIN LED_BUILTIN

void dd_led_setup();
void dd_led_turn_on();
void dd_led_turn_off();

#endif