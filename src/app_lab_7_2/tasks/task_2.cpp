#include "task_2.h"
#include "task_1.h"
#include "task_config.h"
#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <Arduino.h>
#include <stdio.h>

// ===========================================================================
// task_2 – Traffic light FSM  (100 ms period, priority 3)
// ===========================================================================

// --- Shared snapshot --------------------------------------------------------
App72Snapshot_t   g_app72_snapshot       = {};
SemaphoreHandle_t g_app72_snapshot_mutex = NULL;

#define SEM_TICKS pdMS_TO_TICKS(10)

// --- FSM internal state -----------------------------------------------------
static TrafficState_t s_state        = FSM_EV_GREEN;
static uint32_t       s_state_ticks  = 0;   // COND_PERIOD_MS ticks in state
static bool           s_ns_pending   = false;
static bool           s_night_mode   = false;
static bool           s_night_blink  = false;
static uint32_t       s_night_ticks  = 0;

// --- Helper: ticks → ms elapsed in current state ---------------------------
static inline uint32_t state_ms() {
    return s_state_ticks * (uint32_t)COND_PERIOD_MS;
}

// --- Helper: transition to new state ----------------------------------------
static void go_to(TrafficState_t next) {
    s_state       = next;
    s_state_ticks = 0;
}

// --- Drive all 6 LEDs from FSM state ----------------------------------------
static void drive_leds() {
    // All off first
    digitalWrite(PIN_EV_GREEN,  LOW);
    digitalWrite(PIN_EV_YELLOW, LOW);
    digitalWrite(PIN_EV_RED,    LOW);
    digitalWrite(PIN_NS_GREEN,  LOW);
    digitalWrite(PIN_NS_YELLOW, LOW);
    digitalWrite(PIN_NS_RED,    LOW);

    if (s_night_mode) {
        // Both yellow blink in sync
        uint8_t y = s_night_blink ? HIGH : LOW;
        digitalWrite(PIN_EV_YELLOW, y);
        digitalWrite(PIN_NS_YELLOW, y);
        return;
    }

    switch (s_state) {
        case FSM_EV_GREEN:
            digitalWrite(PIN_EV_GREEN, HIGH);
            digitalWrite(PIN_NS_RED,   HIGH);
            break;
        case FSM_EV_YELLOW:
            digitalWrite(PIN_EV_YELLOW, HIGH);
            digitalWrite(PIN_NS_RED,    HIGH);
            break;
        case FSM_ALL_RED_1:
        case FSM_ALL_RED_2:
            digitalWrite(PIN_EV_RED, HIGH);
            digitalWrite(PIN_NS_RED, HIGH);
            break;
        case FSM_NS_GREEN:
            digitalWrite(PIN_EV_RED,  HIGH);
            digitalWrite(PIN_NS_GREEN, HIGH);
            break;
        case FSM_NS_YELLOW:
            digitalWrite(PIN_EV_RED,   HIGH);
            digitalWrite(PIN_NS_YELLOW, HIGH);
            break;
        case FSM_NIGHT:
            break;  // handled above
    }
}

// --- FSM tick ---------------------------------------------------------------
static void fsm_tick() {
    s_state_ticks++;

    // Night mode blink counter (independent of FSM state timer)
    s_night_ticks++;
    if (s_night_ticks >= (NIGHT_BLINK_MS / COND_PERIOD_MS)) {
        s_night_ticks = 0;
        s_night_blink = !s_night_blink;
    }

    if (s_night_mode) {
        // Night mode: only exit on toggle_night (handled in task body)
        return;
    }

    uint32_t ms = state_ms();

    switch (s_state) {
        case FSM_EV_GREEN:
            // Switch if NS request and minimum green time elapsed
            if (s_ns_pending && ms >= EV_GREEN_MIN_MS) {
                go_to(FSM_EV_YELLOW);
            } else if (ms >= EV_GREEN_MS) {
                // No request: cycle anyway after full green time
                go_to(FSM_EV_YELLOW);
            }
            break;

        case FSM_EV_YELLOW:
            if (ms >= EV_YELLOW_MS) go_to(FSM_ALL_RED_1);
            break;

        case FSM_ALL_RED_1:
            if (ms >= ALL_RED_MS) go_to(FSM_NS_GREEN);
            break;

        case FSM_NS_GREEN:
            if (ms >= NS_GREEN_MS) {
                go_to(FSM_NS_YELLOW);
                s_ns_pending = false;   // request served
            }
            break;

        case FSM_NS_YELLOW:
            if (ms >= NS_YELLOW_MS) go_to(FSM_ALL_RED_2);
            break;

        case FSM_ALL_RED_2:
            if (ms >= ALL_RED_MS) go_to(FSM_EV_GREEN);
            break;

        case FSM_NIGHT:
            break;
    }
}

// --- Build LED booleans for snapshot ----------------------------------------
static void leds_for_snap(bool *ev_g, bool *ev_y, bool *ev_r,
                           bool *ns_g, bool *ns_y, bool *ns_r) {
    if (s_night_mode) {
        *ev_g = false; *ev_y = s_night_blink; *ev_r = false;
        *ns_g = false; *ns_y = s_night_blink; *ns_r = false;
        return;
    }
    *ev_g = (s_state == FSM_EV_GREEN);
    *ev_y = (s_state == FSM_EV_YELLOW);
    *ev_r = (s_state == FSM_ALL_RED_1 || s_state == FSM_ALL_RED_2 ||
             s_state == FSM_NS_GREEN  || s_state == FSM_NS_YELLOW);
    *ns_g = (s_state == FSM_NS_GREEN);
    *ns_y = (s_state == FSM_NS_YELLOW);
    *ns_r = (s_state == FSM_EV_GREEN  || s_state == FSM_EV_YELLOW ||
             s_state == FSM_ALL_RED_1 || s_state == FSM_ALL_RED_2);
}

// --- Export snapshot --------------------------------------------------------
static void export_snapshot(bool force_report) {
    if (xSemaphoreTake(g_app72_snapshot_mutex, SEM_TICKS) != pdTRUE) return;

    bool ev_g, ev_y, ev_r, ns_g, ns_y, ns_r;
    leds_for_snap(&ev_g, &ev_y, &ev_r, &ns_g, &ns_y, &ns_r);

    g_app72_snapshot.state       = s_state;
    g_app72_snapshot.ev_green    = ev_g;
    g_app72_snapshot.ev_yellow   = ev_y;
    g_app72_snapshot.ev_red      = ev_r;
    g_app72_snapshot.ns_green    = ns_g;
    g_app72_snapshot.ns_yellow   = ns_y;
    g_app72_snapshot.ns_red      = ns_r;
    g_app72_snapshot.ns_request  = s_ns_pending;
    g_app72_snapshot.night_mode  = s_night_mode;
    g_app72_snapshot.night_blink = s_night_blink;
    g_app72_snapshot.state_ms    = state_ms();
    g_app72_snapshot.uptime_ms   =
        (uint32_t)((uint32_t)xTaskGetTickCount() * portTICK_PERIOD_MS);
    g_app72_snapshot.force_report = force_report;

    xSemaphoreGive(g_app72_snapshot_mutex);
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
void task2_72_setup() {
    g_app72_snapshot_mutex = xSemaphoreCreateMutex();
}

// ---------------------------------------------------------------------------
// Task entry point
// ---------------------------------------------------------------------------
void task2_72_run(void *pvParameters) {
    (void)pvParameters;
    TickType_t next_tick = xTaskGetTickCount();

    for (;;) {
        App72UserCmd_t cmd = task1_72_get_cmd();

        // Apply NS request (latched until served)
        if (cmd.ns_request && !s_night_mode) {
            s_ns_pending = true;
            printf("\rNS REQUEST received – queued\n");
        }

        // Toggle night mode
        if (cmd.toggle_night) {
            s_night_mode  = !s_night_mode;
            s_night_ticks = 0;
            s_night_blink = false;
            if (s_night_mode) {
                printf("\rNIGHT MODE: ON (yellow blink)\n");
                s_state       = FSM_NIGHT;
                s_state_ticks = 0;
            } else {
                printf("\rNIGHT MODE: OFF – resuming EV_GREEN\n");
                s_state       = FSM_EV_GREEN;
                s_state_ticks = 0;
                s_ns_pending  = false;
            }
        }

        fsm_tick();
        drive_leds();
        export_snapshot(cmd.force_report);

        vTaskDelayUntil(&next_tick, pdMS_TO_TICKS(COND_PERIOD_MS));
    }
}