#include "app_lab_3_2.h"
#include "Arduino_FreeRTOS.h"
#include "srv_serial_stdio/srv_serial_stdio.h"
#include "dd_button/dd_button.h"
#include "dd_led/dd_led.h"
#include "tasks/task_1.h"
#include "tasks/task_2.h"
#include "tasks/task_3.h"

void app_lab_3_2_setup()
{
    srv_serial_stdio_setup();
    dd_button_setup();
    dd_led_setup();

    xSemaphore = xSemaphoreCreateBinary();
    xQueue = xQueueCreate(100, sizeof(int));

    xTaskCreate(task_1, "Task 1", 500, NULL, 1, NULL);
    xTaskCreate(task_2, "Task 2", 500, NULL, 1, NULL);
    xTaskCreate(task_3, "Task 3", 500, NULL, 1, NULL);
    vTaskStartScheduler();
}

void app_lab_3_2_loop()
{
    
}