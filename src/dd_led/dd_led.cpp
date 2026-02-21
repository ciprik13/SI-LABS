#include "dd_led.h"

static int led_state        = 0;
static int led_1_state      = 0;
static int led_target       = 0;
static int led_1_target     = 0;

void dd_led_setup() {
    pinMode(LED_PIN,   OUTPUT);
    pinMode(LED_1_PIN, OUTPUT);
    digitalWrite(LED_PIN,   LOW);
    digitalWrite(LED_1_PIN, LOW);
}

void dd_led_set_target(int val)   { led_target   = val; }
void dd_led_1_set_target(int val) { led_1_target = val; }

void dd_led_apply() {
    if (led_target != led_state) {
        led_state = led_target;
        digitalWrite(LED_PIN, led_state ? HIGH : LOW);
    }
    if (led_1_target != led_1_state) {
        led_1_state = led_1_target;
        digitalWrite(LED_1_PIN, led_1_state ? HIGH : LOW);
    }
}

int dd_led_is_on()   { return led_target; }
int dd_led_1_is_on() { return led_1_target; }

void dd_led_turn_on()    { led_target = 1; }
void dd_led_turn_off()   { led_target = 0; }
void dd_led_1_turn_on()  { led_1_target = 1; }
void dd_led_1_turn_off() { led_1_target = 0; }