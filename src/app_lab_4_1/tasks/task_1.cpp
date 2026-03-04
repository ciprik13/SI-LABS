#include "task_1.h"
#include "dd_sns_temperature/dd_sns_temperature.h"
#include <Arduino_FreeRTOS.h>

// ===========================================================================
// Task 1 – Sensor Acquisition  (50 ms strict period via vTaskDelayUntil)
//
// Reads raw ADC value from the potentiometer (used as temperature simulator),
// converts it to voltage and Celsius inside dd_sns_temperature_loop(), and
// stores results under the sensor mutex so other tasks can safely read them.
// ===========================================================================
void task_acquisition(void *pvParameters) {
    (void) pvParameters;

    TickType_t xLastWakeTime = xTaskGetTickCount();

    for (;;) {
        dd_sns_temperature_loop();
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(50));
    }
}