#include "act_binary.h"
#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <Arduino.h>
#include <stdio.h>

// ===========================================================================
// Binary Actuator – Internal (hidden) state
//
// BinState_t is defined only here; no other module can access its fields.
// ===========================================================================

#define BIN_DEBOUNCE_SAMPLES   3     // 3 × 50 ms = 150 ms min persistence
#define MTX_TIMEOUT            pdMS_TO_TICKS(10)

typedef struct {
    int  pin;           // hardware pin
    int  requested;     // latest sanitised request: 0, 1, or -1
    int  pending;       // candidate value being debounced
    int  bounce_count;  // consecutive confirmations of pending
    int  committed;     // debounced state last written to hardware
    bool actuator_on;   // actual pin output
} BinState_t;

static BinState_t        s_bin   = { -1, -1, -1, 0, -1, false };
static SemaphoreHandle_t s_mutex = NULL;

// ---------------------------------------------------------------------------
void act_binary_init(int pin) {
    s_bin.pin = pin;
    s_mutex   = xSemaphoreCreateMutex();
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
}

// ---------------------------------------------------------------------------
void act_binary_request(int state) {
    if (state != 0 && state != 1) return;   // saturation: reject invalid values
    if (xSemaphoreTake(s_mutex, MTX_TIMEOUT) == pdTRUE) {
        s_bin.requested = state;
        xSemaphoreGive(s_mutex);
    } else {
        printf("\r[act_binary] WARNING: mutex timeout in request()\n");
    }
}

// ---------------------------------------------------------------------------
void act_binary_tick() {
    if (xSemaphoreTake(s_mutex, MTX_TIMEOUT) != pdTRUE) {
        printf("\r[act_binary] WARNING: mutex timeout in tick()\n");
        return;
    }

    int req = s_bin.requested;
    s_bin.requested = -1;   // consume

    if (req == 0 || req == 1) {
        // Debounce: require BIN_DEBOUNCE_SAMPLES consecutive identical requests
        if (req == s_bin.pending) {
            s_bin.bounce_count++;
        } else {
            s_bin.pending      = req;
            s_bin.bounce_count = 1;
        }
        if (s_bin.bounce_count >= BIN_DEBOUNCE_SAMPLES) {
            s_bin.committed    = s_bin.pending;
            s_bin.bounce_count = BIN_DEBOUNCE_SAMPLES;  // cap
        }
    }

    // Apply committed state to hardware (single write inside the lock)
    if (s_bin.committed == 1) {
        digitalWrite(s_bin.pin, HIGH);
        s_bin.actuator_on = true;
    } else if (s_bin.committed == 0) {
        digitalWrite(s_bin.pin, LOW);
        s_bin.actuator_on = false;
    }

    xSemaphoreGive(s_mutex);
}

// ---------------------------------------------------------------------------
int act_binary_get_state() {
    int v = -1;
    if (xSemaphoreTake(s_mutex, MTX_TIMEOUT) == pdTRUE) {
        v = s_bin.actuator_on ? 1 : 0;
        xSemaphoreGive(s_mutex);
    } else {
        printf("\r[act_binary] WARNING: mutex timeout in get_state()\n");
    }
    return v;
}

int act_binary_get_pending() {
    int v = -1;
    if (xSemaphoreTake(s_mutex, MTX_TIMEOUT) == pdTRUE) {
        v = s_bin.pending;
        xSemaphoreGive(s_mutex);
    }
    return v;
}

int act_binary_get_bounce_count() {
    int v = 0;
    if (xSemaphoreTake(s_mutex, MTX_TIMEOUT) == pdTRUE) {
        v = s_bin.bounce_count;
        xSemaphoreGive(s_mutex);
    }
    return v;
}

// Configuration getter — no lock needed (compile-time constant)
int act_binary_get_debounce_samples() { return BIN_DEBOUNCE_SAMPLES; }