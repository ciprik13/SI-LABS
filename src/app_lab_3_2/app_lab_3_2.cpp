#include "app_lab_3_2.h"
#include "Arduino_FreeRTOS.h"
#include "srv_serial_stdio/srv_serial_stdio.h"
#include "dd_button/dd_button.h"
#include "dd_led/dd_led.h"
#include "tasks/task_config.h"
#include "tasks/task_1.h"
#include "tasks/task_2.h"
#include "tasks/task_3.h"

/*
 * Tabel de configurare task-uri:
 *   name        — numele task-ului (pentru debugging FreeRTOS)
 *   fn          — pointer la funcția task-ului
 *   period_ms   — recurența (0 = event-driven)
 *   offset_ms   — offset inițial față de pornire
 *   stack_words — dimensiunea stivei în cuvinte
 *   priority    — prioritatea FreeRTOS
 */
static const TaskConfig_t task_table[] = {
    /*  name      fn       period_ms  offset_ms  stack  priority */
    { "Task1",  task_1,       10,        0,       500,     1    },
    { "Task2",  task_2,        0,       20,       500,     1    },
    { "Task3",  task_3,    10000,       40,       500,     1    },
};

#define TASK_COUNT  (sizeof(task_table) / sizeof(task_table[0]))

void app_lab_3_2_setup()
{
    srv_serial_stdio_setup();
    dd_button_setup();
    dd_led_setup();

    xSemaphore = xSemaphoreCreateBinary();  /* semnalizare Task1 → Task2  */
    xMutex     = xSemaphoreCreateMutex();   /* protecție variabile globale */
    xReady     = xSemaphoreCreateBinary();  /* alternare strictă T1→T2    */
    xSemaphoreGive(xReady);                 /* Task1 poate porni primul   */

    /* Creare task-uri din tabelul de configurare */
    for (uint8_t i = 0; i < TASK_COUNT; i++)
    {
        xTaskCreate(task_table[i].fn,
                    task_table[i].name,
                    task_table[i].stack_words,
                    (void *)&task_table[i],   /* config transmisă prin pvParameters */
                    task_table[i].priority,
                    NULL);
    }

    vTaskStartScheduler();
}

void app_lab_3_2_loop()
{
}