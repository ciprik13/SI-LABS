#include <Arduino.h>
#include "app_lab_1_1/app_lab_1_1.h"
#include "app_lab_1_2/app_lab_1_2.h"

#define USE_APP_LAB_1_1 11
#define USE_APP_LAB_1_2 12

#define APP_USED USE_APP_LAB_1_2

void setup() {
  #if APP_USED == USE_APP_LAB_1_1
    app_lab_1_1_setup();
  #elif APP_USED == USE_APP_LAB_1_2
    app_lab_1_2_setup();
  #endif
}

void loop() {
  #if APP_USED == USE_APP_LAB_1_1
    app_lab_1_1_loop();
  #elif APP_USED == USE_APP_LAB_1_2
    app_lab_1_2_loop();
  #endif
}
