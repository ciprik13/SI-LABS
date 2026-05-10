#include "task_2.h"
#include "task_1.h"
#include "task_config.h"
#include "dd_button/dd_button.h"
#include "dd_led/dd_led.h"
#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <Arduino.h>
#include <stdio.h>

// ===========================================================================
// task_2 – Button acquisition + FSM control  (20 ms period, priority 3)
//
// FSM states:
//   LED_OFF        – LED off, waiting for button press
//   LED_ON         – LED on,  waiting for button press or auto-timeout
//
// Transitions:
//   LED_OFF → LED_ON  : validated button press
//   LED_ON  → LED_OFF : validated button press  OR  auto-timeout (10 s)
//
// Bonus behaviour (10%):
//   Auto-timeout: if FSM stays in LED_ON for AUTO_TIMEOUT_MS (10 s) without
//   any button press, it automatically transitions back to LED_OFF.
//   The remaining time is tracked in the snapshot so task_3 can display it.
//
// Debounce: release-edge detection with lockout (see debounce_tick).
// ===========================================================================

// ---------------------------------------------------------------------------
// Shared snapshot
// ---------------------------------------------------------------------------
App71Snapshot_t   g_app71_snapshot       = {
    FSM_LED_OFF, false, false, false, false, 0, false, 0
};
SemaphoreHandle_t g_app71_snapshot_mutex = NULL;

#define SEM_TICKS pdMS_TO_TICKS(10)

// ---------------------------------------------------------------------------
// Debounce state machine
// ---------------------------------------------------------------------------
typedef enum {
    DEB_IDLE = 0,
    DEB_PRESSED,
    DEB_WAIT_RELEASE
} DebState_t;

static DebState_t s_deb_state      = DEB_IDLE;
static int        s_deb_count      = 0;
static uint32_t   s_press_start_ms = 0;
static uint32_t   s_lockout_end_ms = 0;
static uint32_t   s_press_count    = 0;

// ---------------------------------------------------------------------------
// FSM state + auto-timeout
// ---------------------------------------------------------------------------
static FsmState_t s_fsm_state      = FSM_LED_OFF;
static uint32_t   s_led_on_since   = 0;     // ms when LED_ON was entered
static bool       s_timeout_active = false; // true while in LED_ON

// ---------------------------------------------------------------------------
// btn_debounced latch
// ---------------------------------------------------------------------------
static bool s_deb_latch = false;

// ---------------------------------------------------------------------------
// YELLOW blink state
// ---------------------------------------------------------------------------
static TickType_t s_blink_last  = 0;
static bool       s_blink_state = false;

// ---------------------------------------------------------------------------
// now_ms() helper
// ---------------------------------------------------------------------------
static inline uint32_t now_ms() {
    return (uint32_t)((uint32_t)xTaskGetTickCount() * portTICK_PERIOD_MS);
}

// ---------------------------------------------------------------------------
// debounce_tick
// ---------------------------------------------------------------------------
static bool debounce_tick(bool btn_low, bool *out_stuck) {
    bool     fired = false;
    uint32_t now   = now_ms();
    *out_stuck     = false;

    if (now < s_lockout_end_ms) {
        s_deb_state = DEB_IDLE;
        s_deb_count = 0;
        return false;
    }

    switch (s_deb_state) {
        case DEB_IDLE:
            if (btn_low) {
                s_deb_count = 1;
                s_deb_state = DEB_PRESSED;
            }
            break;

        case DEB_PRESSED:
            if (btn_low) {
                s_deb_count++;
                if (s_deb_count >= DEBOUNCE_SAMPLES) {
                    s_press_start_ms = now;
                    s_deb_state      = DEB_WAIT_RELEASE;
                }
            } else {
                s_deb_count = 0;
                s_deb_state = DEB_IDLE;
            }
            break;

        case DEB_WAIT_RELEASE:
            if (btn_low) {
                uint32_t held_ms = now - s_press_start_ms;
                if (held_ms > (uint32_t)STUCK_THRESHOLD_MS) {
                    *out_stuck = true;
                }
            } else {
                fired            = true;
                s_press_count++;
                s_lockout_end_ms = now + (uint32_t)DEBOUNCE_LOCKOUT_MS;
                s_deb_count      = 0;
                s_deb_state      = DEB_IDLE;
            }
            break;
    }

    return fired;
}

// ---------------------------------------------------------------------------
// update_leds
// ---------------------------------------------------------------------------
static void update_leds(FsmState_t state, bool btn_stuck) {
    if (state == FSM_LED_ON) {
        dd_led_turn_on();
        dd_led_1_turn_off();
    } else {
        dd_led_turn_off();
        dd_led_1_turn_on();
    }

    if (btn_stuck) {
        TickType_t now_t = xTaskGetTickCount();
        if ((now_t - s_blink_last) >= pdMS_TO_TICKS(BLINK_PERIOD_MS)) {
            s_blink_last  = now_t;
            s_blink_state = !s_blink_state;
        }
        s_blink_state ? dd_led_2_turn_on() : dd_led_2_turn_off();
    } else {
        dd_led_2_turn_off();
        s_blink_state = false;
    }
}

// ---------------------------------------------------------------------------
// export_snapshot
// ---------------------------------------------------------------------------
static void export_snapshot(FsmState_t fsm_state, bool led_on,
                             bool btn_raw, bool btn_stuck,
                             uint32_t press_count, bool force_report,
                             uint32_t timeout_rem_s) {
    if (xSemaphoreTake(g_app71_snapshot_mutex, SEM_TICKS) != pdTRUE) return;

    g_app71_snapshot.fsm_state      = fsm_state;
    g_app71_snapshot.led_on         = led_on;
    g_app71_snapshot.btn_raw        = btn_raw;
    g_app71_snapshot.btn_debounced  = s_deb_latch;
    g_app71_snapshot.btn_stuck      = btn_stuck;
    g_app71_snapshot.press_count    = press_count;
    g_app71_snapshot.force_report   = force_report;
    g_app71_snapshot.timeout_rem_s  = timeout_rem_s;
    g_app71_snapshot.uptime_ms      = now_ms();

    s_deb_latch = false;

    xSemaphoreGive(g_app71_snapshot_mutex);
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
void task2_71_setup() {
    g_app71_snapshot_mutex = xSemaphoreCreateMutex();
}

// ---------------------------------------------------------------------------
// Task entry point
// ---------------------------------------------------------------------------
void task2_71_run(void *pvParameters) {
    (void)pvParameters;
    TickType_t next_tick = xTaskGetTickCount();

    for (;;) {
        App71UserCmd_t cmd    = task1_71_get_cmd();
        uint32_t       now    = now_ms();

        bool btn_raw     = (dd_button_is_pressed() != 0);
        bool btn_stuck   = false;
        bool press_event = debounce_tick(btn_raw, &btn_stuck);

        // --- FSM transitions ------------------------------------------------
        if (press_event) {
            s_deb_latch = true;
            if (s_fsm_state == FSM_LED_OFF) {
                s_fsm_state      = FSM_LED_ON;
                s_led_on_since   = now;
                s_timeout_active = true;
            } else {
                s_fsm_state      = FSM_LED_OFF;
                s_timeout_active = false;
            }
        }

        // --- Auto-timeout: LED_ON → LED_OFF after AUTO_TIMEOUT_MS ----------
        if (s_timeout_active && s_fsm_state == FSM_LED_ON) {
            uint32_t elapsed = now - s_led_on_since;
            if (elapsed >= (uint32_t)AUTO_TIMEOUT_MS) {
                s_fsm_state      = FSM_LED_OFF;
                s_timeout_active = false;
                printf("\rAUTO-TIMEOUT: LED turned OFF after %us\n",
                       AUTO_TIMEOUT_MS / 1000);
            }
        }

        // --- Compute remaining timeout seconds (0 when OFF) -----------------
        uint32_t timeout_rem_s = 0;
        if (s_timeout_active && s_fsm_state == FSM_LED_ON) {
            uint32_t elapsed = now - s_led_on_since;
            uint32_t rem_ms  = (elapsed < (uint32_t)AUTO_TIMEOUT_MS)
                               ? ((uint32_t)AUTO_TIMEOUT_MS - elapsed)
                               : 0;
            timeout_rem_s = (rem_ms + 999) / 1000;  // ceiling to nearest second
        }

        update_leds(s_fsm_state, btn_stuck);

        export_snapshot(
            s_fsm_state, (s_fsm_state == FSM_LED_ON),
            btn_raw, btn_stuck,
            s_press_count, cmd.force_report,
            timeout_rem_s
        );

        vTaskDelayUntil(&next_tick, pdMS_TO_TICKS(COND_PERIOD_MS));
    }
}