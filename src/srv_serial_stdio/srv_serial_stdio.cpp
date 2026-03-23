#include "srv_serial_stdio.h"
#include "Arduino.h"

#include <stdio.h>

int srv_serial_put_char(char ch, FILE *f)
{
    return Serial.write(ch);
}

int srv_serial_get_char(FILE *f)
{
    while (!Serial.available())
        ;
    return Serial.read();
}

int srv_serial_stdio_try_get_char(char *out_ch)
{
    if (out_ch == NULL)
        return 0;

    if (!Serial.available())
        return 0;

    *out_ch = (char)Serial.read();
    return 1;
}

void srv_serial_stdio_setup()
{
    Serial.begin(9600);
    
    FILE *srv_serial_stream = fdevopen(srv_serial_put_char, srv_serial_get_char);

    stdin = srv_serial_stream;
    stdout = srv_serial_stream;
}