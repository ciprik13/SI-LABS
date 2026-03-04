#ifndef DD_SNS_ANGLE_H
#define DD_SNS_ANGLE_H

// Physical range of the rotary angle sensor (KY-040 / 37-in-1 analog)
#define DD_SNS_ANGLE_MIN    0
#define DD_SNS_ANGLE_MAX    270
#define DD_SNS_ANGLE_CENTER (DD_SNS_ANGLE_MAX / 2)   // 135 – zero reference

void dd_sns_angle_setup();
void dd_sns_angle_loop();   // call from acquisition task (every 50 ms)

// Mutex-protected getters – safe to call from any task
int  dd_sns_angle_get_raw();
int  dd_sns_angle_get_voltage();   // millivolts, 0..5000
int  dd_sns_angle_get_value();     // centred angle, -135..+135 degrees

#endif