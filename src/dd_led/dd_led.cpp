#include "dd_led.h"

void dd_led_setup(){
    pinMode(LED_PIN, OUTPUT);
}

void dd_led_turn_on(){
    digitalWrite(LED_PIN, HIGH);
}

void dd_led_turn_off(){
    digitalWrite(LED_PIN, LOW);
}

int dd_led_is_on(){
    return (digitalRead(LED_PIN) == HIGH)?1:0;
}
