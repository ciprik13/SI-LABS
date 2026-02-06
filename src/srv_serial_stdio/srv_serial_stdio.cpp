#include "srv_serial_stdio.h"
#include "Arduino.h"

#include <stdio.h>

int srv_serial_put_char(char ch, FILE *f)
{
    return Serial.write(ch);
}

int srv_serial_get_char(FILE *f)
{
    while (!Serial.available());
        char ch = Serial.read();
    if (ch == '\r') {
        Serial.write('\r');
        return '\n';
    }
    Serial.write(ch);
    return ch;
}

void srv_serial_stdio_setup()
{
    Serial.begin(9600);
    
    FILE *srv_serial_stream = fdevopen(srv_serial_put_char, srv_serial_get_char);

    stdin = srv_serial_stream;
    stdout = srv_serial_stream;
}