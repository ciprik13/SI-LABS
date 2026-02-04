#include <Arduino.h>
#include "app_lab_1_1/app_lab_1_1.h"

#define USE_APP_LAB_1_1 11

#define APP_USED USE_APP_LAB_1_1

void setup(){
  #if APP_USED == USE_APP_LAB_1_1
    app_lab_1_1_setup();
  #endif
}

void loop(){
  #if APP_USED == USE_APP_LAB_1_1
    app_lab_1_1_loop();
  #endif
}
