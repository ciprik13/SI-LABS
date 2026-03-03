#ifndef DD_SNS_ANGLE_H
#define DD_SNS_ANGLE_H

#define DD_SNS_ANGLE_MIN 0
#define DD_SNS_ANGLE_MAX 270
#define DD_SNS_ANGLE_WARN_THRESHOLD 100

void dd_sns_angle_setup();
void dd_sns_angle_loop();

int dd_sns_angle_get_value();

#endif