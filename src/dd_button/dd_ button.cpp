#include "dd_button/dd_button.h"

void dd_button_setup() {
    pinMode(BUTTON_PIN,   INPUT_PULLUP);
    pinMode(BUTTON_1_PIN, INPUT_PULLUP);
    pinMode(BUTTON_2_PIN, INPUT_PULLUP);
}

int dd_button_is_pressed() {
    return digitalRead(BUTTON_PIN) == LOW;
}

int dd_button_1_is_pressed() {
    return digitalRead(BUTTON_1_PIN) == LOW;
}

int dd_button_2_is_pressed() {
    return digitalRead(BUTTON_2_PIN) == LOW;
}