#include "app_lab_4_1.h"
#include "Arduino_FreeRTOS.h"
#include "srv_serial_stdio/srv_serial_stdio.h"
#include "dd_sns_angle/dd_sns_angle.h"
#include "ed_potentiometer/ed_potentiometer.h"
#include <Arduino.h>

void app_lab_4_1_angle_sns_task(void *pvParameters) {
  (void) pvParameters;

  // Offset so the acquisition task does not start at the same time as the report task
  vTaskDelay(500 / portTICK_PERIOD_MS);
  TickType_t xLastWakeTime = xTaskGetTickCount();

  for (;;) {
    dd_sns_angle_loop();
    vTaskDelayUntil(&xLastWakeTime, 1000 / portTICK_PERIOD_MS);
  }
}

void app_lab_4_1_system_report_task(void *pvParameters) {
  (void) pvParameters;

  for (;;) {
    int raw     = ed_potentiometer_get_raw();
    int voltage = ed_potentiometer_get_voltage();
    int angle   = dd_sns_angle_get_value();

    printf("------------------------------\n");
    printf("System report: All systems nominal.\n");
    printf("Sensor RAW:     %4d\n",    raw);
    printf("Sensor voltage: %4d mV\n", voltage);
    printf("Sensor angle:   %4d deg\n", angle);

    if (angle > DD_SNS_ANGLE_WARN_THRESHOLD || angle < -DD_SNS_ANGLE_WARN_THRESHOLD) {
      printf("*** WARNING: Angle out of safe range! ***\n");
    }

    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}

void app_lab_4_1_setup() {
  srv_serial_stdio_setup();
  dd_sns_angle_setup();

  // higher priority for acquisition, lower for reporting
  xTaskCreate(app_lab_4_1_angle_sns_task,   "AngleSnsTask",    256, NULL, 2, NULL);
  xTaskCreate(app_lab_4_1_system_report_task, "SystemReportTask", 256, NULL, 1, NULL);
}

void app_lab_4_1_loop() {
  // FreeRTOS takes over; loop is intentionally empty.
}