#ifndef SRV_SERIAL_STDIO_H_
#define SRV_SERIAL_STDIO_H_

#include <stdio.h>

int srv_serial_put_char(char ch, FILE *f);
int srv_serial_get_char(FILE *f);
int srv_serial_stdio_try_get_char(char *out_ch);

void srv_serial_stdio_setup();

#endif