#ifndef TASK_CONFIG_H
#define TASK_CONFIG_H

#include <Arduino_FreeRTOS.h>
#include <semphr.h>

// ===========================================================================
// Sensor 1 – Potentiometer (analog)  thresholds
// ===========================================================================
#define ALERT_THRESHOLD_HIGH   50    // °C  – alert ON  edge
#define ALERT_THRESHOLD_LOW    45    // °C  – alert OFF edge
#define ANTIBOUNCE_SAMPLES      5    // 5 x 50 ms = 250 ms min persistence

// ===========================================================================
// Sensor 2 – DHT22 (digital)  thresholds
// ===========================================================================
#define ALERT2_THRESHOLD_HIGH  30    // °C  – alert ON  edge
#define ALERT2_THRESHOLD_LOW   25    // °C  – alert OFF edge
#define ANTIBOUNCE2_SAMPLES     5    // 5 x 50 ms = 250 ms min persistence

// ===========================================================================
// Shared conditioning state (owned by task_2, read by task_3)
// ===========================================================================
typedef struct {
    bool alert_active;   // committed, debounced alert flag
    bool pending_state;  // candidate state being counted
    int  bounce_count;   // consecutive samples confirming pending_state
} CondState_t;

// Sensor 1 state  (defined in task_2.cpp)
extern CondState_t       g_cond;
extern SemaphoreHandle_t g_cond_mutex;

// Sensor 2 state  (defined in task_2.cpp)
extern CondState_t       g_cond2;
extern SemaphoreHandle_t g_cond2_mutex;

#endif
