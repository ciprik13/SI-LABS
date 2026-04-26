#include "task_2.h"
#include "task_1.h"
#include "task_config.h"
#include "act_binary/act_binary.h"
#include "ed_dht/ed_dht.h"
#include "dd_led/dd_led.h"
#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <Arduino.h>
#include <stdio.h>
#include <stdlib.h>

// ===========================================================================
// task_2 – DHT acquisition + ON-OFF hysteresis control  (25 ms period, priority 2)
//
// ON-OFF hysteresis (HEATING):
//   relay ON  when temp < thresh_on  (= SP - HYST/2)
//   relay OFF when temp > thresh_off (= SP + HYST/2)
//   no change inside the band  → prevents chattering
//
// Bonus LED behaviour:
//   YELLOW (pin 11) – mirrors relay state (heater active)
//   GREEN  (pin 12) – ON when relay OFF (comfortable)
//   RED    (pin  9) – blinks 250 ms when |temp - SP| > 2 × HYST
// ===========================================================================

// ---------------------------------------------------------------------------
// Shared snapshot
// ---------------------------------------------------------------------------
App61Snapshot_t   g_app61_snapshot       = {
    0, 0, 0,
    SP_DEFAULT, HYST_DEFAULT,
    SP_DEFAULT - HYST_DEFAULT / 2,
    SP_DEFAULT + HYST_DEFAULT / 2,
    false, false,
    false, false, 0
};
SemaphoreHandle_t g_app61_snapshot_mutex = NULL;

#define SEM_TICKS pdMS_TO_TICKS(10)

// ---------------------------------------------------------------------------
// Persistent hysteresis state
// ---------------------------------------------------------------------------
static bool s_relay_requested = false;   // last hysteresis decision

// ---------------------------------------------------------------------------
// Compute thresholds
// Note: integer arithmetic; HYST/2 truncates for odd values.
// thresh_on  = SP - HYST/2
// thresh_off = SP + HYST/2 + (HYST % 2)   (ceiling for odd band)
// ---------------------------------------------------------------------------
static void compute_thresholds(int sp, int hyst, int *thresh_on, int *thresh_off) {
    *thresh_on  = sp - hyst / 2;
    *thresh_off = sp + hyst / 2 + (hyst % 2);
}

// ---------------------------------------------------------------------------
// ON-OFF hysteresis state machine (HEATING)
// Returns the new relay request, holding the previous if within the band.
// ---------------------------------------------------------------------------
static bool hysteresis_tick(int temp, int thresh_on, int thresh_off, bool prev_state) {
    if (temp < thresh_on)  return true;   // too cold → heater ON
    if (temp > thresh_off) return false;  // warm enough → heater OFF
    return prev_state;                    // inside band → hold
}

// ---------------------------------------------------------------------------
// Extreme deviation check: |temp - SP| > EXTREME_DEV_FACTOR × HYST
// ---------------------------------------------------------------------------
static bool is_extreme(int temp, int sp, int hyst) {
    int dev = temp - sp;
    if (dev < 0) dev = -dev;
    return (dev > EXTREME_DEV_FACTOR * hyst);
}

// ---------------------------------------------------------------------------
// LED update
//   RED    – blinks at BLINK_PERIOD_MS when extreme deviation active
//   YELLOW – solid ON when relay ON
//   GREEN  – solid ON when relay OFF
//
// dd_led_setup_with_pins maps: LED0=RED, LED1=GREEN, LED2=YELLOW
// so:  dd_led_turn_on/off  → RED
//      dd_led_1_turn_on/off → GREEN
//      dd_led_2_turn_on/off → YELLOW
// ---------------------------------------------------------------------------
static TickType_t s_blink_last  = 0;
static bool       s_blink_state = false;

static void update_leds(bool relay_on, bool extreme) {
    // YELLOW: follows relay
    relay_on ? dd_led_2_turn_on() : dd_led_2_turn_off();

    // GREEN: inverse of relay
    relay_on ? dd_led_1_turn_off() : dd_led_1_turn_on();

    // RED: blink when extreme deviation, OFF otherwise
    if (extreme) {
        TickType_t now = xTaskGetTickCount();
        if ((now - s_blink_last) >= pdMS_TO_TICKS(BLINK_PERIOD_MS)) {
            s_blink_last  = now;
            s_blink_state = !s_blink_state;
        }
        s_blink_state ? dd_led_turn_on() : dd_led_turn_off();
    } else {
        dd_led_turn_off();
        s_blink_state = false;
    }
}

// ---------------------------------------------------------------------------
// Export snapshot (single atomic block)
// ---------------------------------------------------------------------------
static void export_snapshot(int temp_raw, int temp_c, int humidity,
                             int sp, int hyst,
                             int thresh_on, int thresh_off,
                             bool relay_on, bool relay_pending,
                             bool extreme, bool force_report) {
    if (xSemaphoreTake(g_app61_snapshot_mutex, SEM_TICKS) != pdTRUE) return;

    g_app61_snapshot.temp_raw      = temp_raw;
    g_app61_snapshot.temp_c        = temp_c;
    g_app61_snapshot.humidity      = humidity;
    g_app61_snapshot.sp            = sp;
    g_app61_snapshot.hyst          = hyst;
    g_app61_snapshot.thresh_on     = thresh_on;
    g_app61_snapshot.thresh_off    = thresh_off;
    g_app61_snapshot.relay_on      = relay_on;
    g_app61_snapshot.relay_pending = relay_pending;
    g_app61_snapshot.extreme_dev   = extreme;
    g_app61_snapshot.force_report  = force_report;
    g_app61_snapshot.uptime_ms     =
        (uint32_t)((uint32_t)xTaskGetTickCount() * portTICK_PERIOD_MS);

    xSemaphoreGive(g_app61_snapshot_mutex);
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
void task2_61_setup() {
    g_app61_snapshot_mutex = xSemaphoreCreateMutex();
    ed_dht_setup();
}

// ---------------------------------------------------------------------------
// Task entry point
// ---------------------------------------------------------------------------
void task2_61_run(void *pvParameters) {
    (void)pvParameters;
    TickType_t next_tick = xTaskGetTickCount();

    for (;;) {
        App61UserCmd_t cmd = task1_61_get_cmd();

        // --- Acquire sensor (throttled inside ed_dht_loop) ------------------
        ed_dht_loop();
        int temp_raw = ed_dht_get_raw();       // ×10 for 0.1°C resolution
        int temp_c   = ed_dht_get_celsius();
        int humidity = ed_dht_get_humidity();

        // --- Plausibility gate: skip until sensor gives a non-zero read ------
        // Wokwi's DHT22 returns raw=0 / humidity=0 while initialising.
        // A real DHT22 never returns exactly 0 for both simultaneously
        // (room temp ≠ 0°C AND humidity ≠ 0 %RH in any real environment).
        // Gate: accept as soon as EITHER temp_raw != 0 OR humidity != 0,
        // and the values are within DHT22 physical limits (-400..800, 0..100).
        // Until the gate passes, hold relay OFF and publish nothing.
        static bool s_sensor_ready = false;
        bool in_range = (temp_raw >= -400 && temp_raw <= 800 &&
                         humidity >=    0 && humidity <=  100);
        bool non_zero = (temp_raw != 0 || humidity != 0);
        if (in_range && non_zero) s_sensor_ready = true;
        if (!s_sensor_ready) {
            vTaskDelayUntil(&next_tick, pdMS_TO_TICKS(COND_PERIOD_MS));
            continue;
        }

        // --- Thresholds from SP and HYST ------------------------------------
        int thresh_on, thresh_off;
        compute_thresholds(cmd.sp, cmd.hyst, &thresh_on, &thresh_off);

        // --- ON-OFF hysteresis state machine --------------------------------
        s_relay_requested = hysteresis_tick(temp_c, thresh_on, thresh_off,
                                             s_relay_requested);

        // --- Drive relay (debounced) ----------------------------------------
        act_binary_request(s_relay_requested ? 1 : 0);
        act_binary_tick();
        bool relay_on      = (act_binary_get_state()   == 1);
        bool relay_pending = (act_binary_get_pending()  != act_binary_get_state());

        // --- Extreme deviation flag -----------------------------------------
        bool extreme = is_extreme(temp_c, cmd.sp, cmd.hyst);

        // --- Status LEDs ----------------------------------------------------
        update_leds(relay_on, extreme);

        // --- Publish snapshot -----------------------------------------------
        export_snapshot(temp_raw, temp_c, humidity,
                        cmd.sp, cmd.hyst,
                        thresh_on, thresh_off,
                        relay_on, relay_pending,
                        extreme, cmd.force_report);

        vTaskDelayUntil(&next_tick, pdMS_TO_TICKS(COND_PERIOD_MS));
    }
}