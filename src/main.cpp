#include <Arduino.h>
#include "app_lab_3_1/app_lab_3_1.h"
#include "app_lab_2_1/app_lab_2_1.h"
#include "app_lab_1_2/app_lab_1_2.h"
#include "app_lab_1_1/app_lab_1_1.h"
#include "app_lab_3_2/app_lab_3_2.h"

#define LAB_1_1 11
#define LAB_1_2 12
#define LAB_2_1 21
#define LAB_3_1 31
#define LAB_3_2 32

#define ACTIVE_APP LAB_3_1

void setup() {
  #if ACTIVE_APP == LAB_3_2
    app_lab_3_2_setup();
  #elif ACTIVE_APP == LAB_3_1
    app_lab_3_1_setup();
  #elif ACTIVE_APP == LAB_2_1
    app_lab_2_1_setup();
  #elif ACTIVE_APP == LAB_1_2
    app_lab_1_2_setup();
  #elif ACTIVE_APP == LAB_1_1
    app_lab_1_1_setup();
  #endif
}

void loop() {
  #if ACTIVE_APP == LAB_3_2
    app_lab_3_2_loop();
  #elif ACTIVE_APP == LAB_3_1
    app_lab_3_1_loop();
  #elif ACTIVE_APP == LAB_2_1
    app_lab_2_1_loop();
  #elif ACTIVE_APP == LAB_1_2
    app_lab_1_2_loop();
  #elif ACTIVE_APP == LAB_1_1
    app_lab_1_1_loop();
  #endif
}
