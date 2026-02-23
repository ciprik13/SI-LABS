#include "dd_button/dd_button.h"

void dd_button_setup() {
    pinMode(BTN_2_PIN, INPUT_PULLUP);
    pinMode(BTN_1_PIN, INPUT_PULLUP);
    pinMode(BTN_PIN,   INPUT_PULLUP);
}

int dd_button_1_is_pressed() {
    return digitalRead(BTN_1_PIN) == LOW;
}

int dd_button_2_is_pressed() {
    return digitalRead(BTN_2_PIN) == LOW;
}

int dd_button_is_pressed() {
    return digitalRead(BTN_PIN) == LOW;
}