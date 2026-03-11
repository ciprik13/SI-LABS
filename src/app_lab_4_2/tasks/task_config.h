#ifndef APP_LAB_4_2_TASK_CONFIG_H
#define APP_LAB_4_2_TASK_CONFIG_H

#include <Arduino_FreeRTOS.h>
#include <semphr.h>

// ===========================================================================
// Sensor 1 – Potentiometer (analog)  saturation + thresholds
// ===========================================================================
#define SAT_LOW_1                  0     // °C  clamp lo
#define SAT_HIGH_1               100     // °C  clamp hi
#define ALERT42_THRESHOLD_HIGH_1   50    // °C  alert ON  edge
#define ALERT42_THRESHOLD_LOW_1    45    // °C  alert OFF edge (hysteresis)
#define ANTIBOUNCE42_SAMPLES_1      5    // 5 × 50 ms = 250 ms min persistence

// ===========================================================================
// Sensor 2 – DHT11 / DHT22  saturation + thresholds
//
// Ajustează limitele de saturare în funcție de senzor:
//   DHT22: -40..80 °C  |  DHT11: 0..50 °C
// ===========================================================================
// #define SAT_LOW_2   -40   // ← DHT22 (Wokwi / simulator)
// #define SAT_HIGH_2   80   // ← DHT22
#define SAT_LOW_2         0  // ← DHT11 (placă fizică)
#define SAT_HIGH_2       50  // ← DHT11
#define ALERT42_THRESHOLD_HIGH_2   30    // °C  alert ON  edge
#define ALERT42_THRESHOLD_LOW_2    25    // °C  alert OFF edge (hysteresis)
#define ANTIBOUNCE42_SAMPLES_2      5    // 5 × 50 ms = 250 ms min persistence

// ===========================================================================
// Signal conditioning filter parameters (shared for both sensors)
//
// Pipeline per sensor (each 50 ms acquisition tick):
//   raw (driver) → saturation → median filter → weighted avg → threshold/debounce
//
// FILTER_WINDOW  samples kept in circular buffer for median + weighted avg.
// Linear weights: oldest sample weight = 1, newest = FILTER_WINDOW.
//   Total weight = FILTER_WINDOW * (FILTER_WINDOW + 1) / 2
// ===========================================================================
#define FILTER_WINDOW               7    // 7 × 50 ms = 350 ms history

// ===========================================================================
// Shared conditioning result (owned by task42_conditioning, read by task42_report)
// Contains all intermediate pipeline stages so the report can display them.
// ===========================================================================
typedef struct {
    int  raw;           // direct driver read (°C)
    int  saturated;     // after saturation clamping
    int  median;        // after median (salt-and-pepper) filter
    int  weighted;      // after weighted moving average – used for threshold
    bool alert_active;  // committed, debounced alert
    bool pending_state; // candidate state being counted
    int  bounce_count;  // consecutive confirmations of pending_state
} CondFull42_t;

// Sensor 1 (defined in task_2.cpp)
extern CondFull42_t      g42_cond1;
extern SemaphoreHandle_t g42_cond1_mutex;

// Sensor 2 (defined in task_2.cpp)
extern CondFull42_t      g42_cond2;
extern SemaphoreHandle_t g42_cond2_mutex;

#endif // APP_LAB_4_2_TASK_CONFIG_H
