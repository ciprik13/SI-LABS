#include "task_1.h"
#include "dd_sns_temperature/dd_sns_temperature.h"
#include "dd_sns_dht/dd_sns_dht.h"
#include "dd_sns_gas/dd_sns_gas.h"
#include <Arduino_FreeRTOS.h>

// ===========================================================================
// Task 1 – Sensor Acquisition  (50 ms strict period via vTaskDelayUntil)
//
// S1: Reads raw ADC from potentiometer, converts to °C via dd_sns_temperature.
// S2: Calls dd_sns_dht_loop() for DHT22 (driver throttles physical bus reads
//     internally to ≥ 2 s; always returns a valid cached value between reads).
// ===========================================================================
void task42_acquisition(void *pvParameters) {
    (void) pvParameters;

    TickType_t xLastWakeTime = xTaskGetTickCount();

    for (;;) {
        dd_sns_temperature_loop();   // S1 – analog potentiometer
        dd_sns_dht_loop();           // S2 – digital DHT11/DHT22
        dd_sns_gas_loop();           // S3 – analog gas sensor (MQ-2, pin A1)
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(50));
    }
}
