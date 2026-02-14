#include "srv_stdio_keypad.h"

#include <stdio.h>
#include <Keypad.h>

#define SRV_KEYPAD_REPEAT_DELAY 100

const byte ROWS = 4;
const byte COLS = 4;

char hexaKeys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {3, 2, 1, 0};
byte colPins[COLS] = {7, 6, 5, 4};

Keypad customKeypad = Keypad( makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS); 

int srv_stdio_keypad_get_key(FILE *stream) {
    char customKey;
    do {
        customKey = customKeypad.getKey();
    } while (customKey == NO_KEY);

    return (int)customKey;   
}

void srv_stdio_keypad_setup() {
  FILE *srv_stdio_keypad_stream = fdevopen(NULL, srv_stdio_keypad_get_key);

   if (srv_stdio_keypad_stream != NULL) {
        stdin = srv_stdio_keypad_stream;
    }
}