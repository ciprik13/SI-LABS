#include "dd_led/dd_led.h"

static int red_target    = 0;
static int green_target  = 0;
static int yellow_target = 0;

void dd_led_setup() {
    pinMode(LED_RED_PIN,    OUTPUT);
    pinMode(LED_GREEN_PIN,  OUTPUT);
    pinMode(LED_YELLOW_PIN, OUTPUT);
    digitalWrite(LED_RED_PIN,    LOW);
    digitalWrite(LED_GREEN_PIN,  LOW);
    digitalWrite(LED_YELLOW_PIN, LOW);
}

void dd_led_set_target(int val)   { red_target    = val; }
void dd_led_1_set_target(int val) { green_target  = val; }
void dd_led_2_set_target(int val) { yellow_target = val; }

int dd_led_is_on()   { return red_target; }
int dd_led_1_is_on() { return green_target; }
int dd_led_2_is_on() { return yellow_target; }

void dd_led_turn_on()    { red_target   = 1; }
void dd_led_turn_off()   { red_target   = 0; }
void dd_led_1_turn_on()  { green_target = 1; }
void dd_led_1_turn_off() { green_target = 0; }

void dd_led_apply() {
    digitalWrite(LED_RED_PIN,    red_target    ? HIGH : LOW);
    digitalWrite(LED_GREEN_PIN,  green_target  ? HIGH : LOW);
    digitalWrite(LED_YELLOW_PIN, yellow_target ? HIGH : LOW);
}