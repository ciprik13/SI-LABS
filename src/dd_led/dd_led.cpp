#include "dd_led.h"

static const uint8_t led_pins[] = {LED_RED_PIN, LED_GREEN_PIN};

void dd_led_setup() {
    pinMode(LED_RED_PIN, OUTPUT);
    pinMode(LED_GREEN_PIN, OUTPUT);
    dd_led_turn_off(LED_RED);
    dd_led_turn_off(LED_GREEN);
}

void dd_led_turn_on(int led_id) {
    digitalWrite(led_pins[led_id], HIGH);
}

void dd_led_turn_off(int led_id) {
    digitalWrite(led_pins[led_id], LOW);
}

int dd_led_is_on(int led_id) {
    return (digitalRead(led_pins[led_id]) == HIGH) ? 1 : 0;
}