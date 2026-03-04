#ifndef TASK_CONFIG_H
#define TASK_CONFIG_H

#include <Arduino_FreeRTOS.h>
#include <semphr.h>

// ===========================================================================
// Threshold / hysteresis / antibounce configuration  (temperature, Celsius)
// ===========================================================================
// Alert ON  when temperature > ALERT_THRESHOLD_HIGH
// Alert OFF when temperature < ALERT_THRESHOLD_LOW  (5 °C hysteresis band)
#define ALERT_THRESHOLD_HIGH   50    // °C  – alert ON  edge
#define ALERT_THRESHOLD_LOW    45    // °C  – alert OFF edge
#define ANTIBOUNCE_SAMPLES      5    // 5 x 50 ms = 250 ms min persistence

// ===========================================================================
// Shared conditioning state (owned by task_2, read by task_3)
// ===========================================================================
typedef struct {
    bool alert_active;   // committed, debounced alert flag
    bool pending_state;  // candidate state being counted
    int  bounce_count;   // consecutive samples confirming pending_state
} CondState_t;

// Defined in task_2.cpp
extern CondState_t       g_cond;
extern SemaphoreHandle_t g_cond_mutex;

#endif
