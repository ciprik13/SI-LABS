#include "dd_led/dd_led.h"

static int red_target    = 0;
static int green_target  = 0;
static int yellow_target = 0;
static uint8_t red_pin_cfg    = LED_RED_PIN;
static uint8_t green_pin_cfg  = LED_GREEN_PIN;
static uint8_t yellow_pin_cfg = LED_YELLOW_PIN;

void dd_led_setup_with_pins(uint8_t red_pin, uint8_t green_pin, uint8_t yellow_pin) {
    red_pin_cfg = red_pin;
    green_pin_cfg = green_pin;
    yellow_pin_cfg = yellow_pin;

    pinMode(red_pin_cfg,    OUTPUT);
    pinMode(green_pin_cfg,  OUTPUT);
    pinMode(yellow_pin_cfg, OUTPUT);
    digitalWrite(red_pin_cfg,    LOW);
    digitalWrite(green_pin_cfg,  LOW);
    digitalWrite(yellow_pin_cfg, LOW);
}

void dd_led_setup() {
    dd_led_setup_with_pins(LED_RED_PIN, LED_GREEN_PIN, LED_YELLOW_PIN);
}

void dd_led_set_target(int val)   { red_target    = val; }
void dd_led_1_set_target(int val) { green_target  = val; }
void dd_led_2_set_target(int val) { yellow_target = val; }

int dd_led_is_on()   { return red_target; }
int dd_led_1_is_on() { return green_target; }
int dd_led_2_is_on() { return yellow_target; }

void dd_led_turn_on()    { red_target    = 1; digitalWrite(red_pin_cfg,    HIGH); }
void dd_led_turn_off()   { red_target    = 0; digitalWrite(red_pin_cfg,    LOW);  }
void dd_led_1_turn_on()  { green_target  = 1; digitalWrite(green_pin_cfg,  HIGH); }
void dd_led_1_turn_off() { green_target  = 0; digitalWrite(green_pin_cfg,  LOW);  }
void dd_led_2_turn_on()  { yellow_target = 1; digitalWrite(yellow_pin_cfg, HIGH); }
void dd_led_2_turn_off() { yellow_target = 0; digitalWrite(yellow_pin_cfg, LOW);  }

void dd_led_apply() {
    digitalWrite(red_pin_cfg,    red_target    ? HIGH : LOW);
    digitalWrite(green_pin_cfg,  green_target  ? HIGH : LOW);
    digitalWrite(yellow_pin_cfg, yellow_target ? HIGH : LOW);
}