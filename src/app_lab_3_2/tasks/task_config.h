#ifndef APP_LAB_3_2_TASK_CONFIG_H
#define APP_LAB_3_2_TASK_CONFIG_H

#include "Arduino_FreeRTOS.h"

/*
 * TaskConfig_t — tabel de configurare task-uri
 *   period_ms   : recurența task-ului (0 = event-driven, fără perioadă fixă)
 *   offset_ms   : întârziere inițială înainte de prima execuție
 *   fn          : pointer la funcția task-ului
 *   stack_words : dimensiunea stivei (în cuvinte)
 *   priority    : prioritatea FreeRTOS
 */
typedef struct {
    const char*    name;
    TaskFunction_t fn;
    uint16_t       period_ms;
    uint16_t       offset_ms;
    uint16_t       stack_words;
    UBaseType_t    priority;
} TaskConfig_t;

#endif
