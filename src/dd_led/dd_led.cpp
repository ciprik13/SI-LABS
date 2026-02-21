#include "dd_led.h"

void dd_led_setup() {
    pinMode(LED_PIN, OUTPUT);
    dd_led_turn_off();
}

void dd_led_turn_on() {
    digitalWrite(LED_PIN, HIGH);
}

void dd_led_1_turn_on() {
    digitalWrite(LED_1_PIN, HIGH);
}

void dd_led_turn_off() {
    digitalWrite(LED_PIN, LOW);
}

void dd_led_1_turn_off() {
    digitalWrite(LED_1_PIN, LOW);
}

int dd_led_is_on() {
    return (digitalRead(LED_PIN) == HIGH) ? 1 : 0;
}

int dd_led_1_is_on() {
    return (digitalRead(LED_1_PIN) == HIGH) ? 1 : 0;
}