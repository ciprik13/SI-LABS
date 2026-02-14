#ifndef SRV_STDIO_KEYPAD_H
#define SRV_STDIO_KEYPAD_H

#include <stdio.h>

void srv_stdio_keypad_setup();
int srv_stdio_keypad_get_key(FILE *stream);

#endif