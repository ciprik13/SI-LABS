#include "task_2.h"
#include "task_1.h"
#include "task_config.h"
#include "act_binary/act_binary.h"
#include "act_analog/act_analog.h"
#include "dd_led/dd_led.h"
#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <Arduino.h>
#include <stdio.h>

// ===========================================================================
// task_2 – Signal conditioning + actuator drive  (25 ms period, priority 2)
//
// Signal conditioning pipeline:
//
//   [AUTO]   pot ADC 0-1023 → map to 0-255 ──┐
//   [MANUAL] pwm_manual 0-255 ───────────────┤
//                                             ↓
//                                       saturate [0..255]
//                                             ↓
//                                       median filter (3-tap)
//                                             ↓
//                                       weighted average (70/30)
//                                             ↓
//                                       ramp limiter (±10 PWM/tick)
//                                             ↓
//                                       analogWrite → L298N ENA
//
// Relay LED:
//   act_binary drives PIN_RELAY (relay coil).
//   The relay's NO contact is wired to a load LED.
//   No separate digitalWrite needed — relay state = LED state.
// ===========================================================================

// ---------------------------------------------------------------------------
// Shared snapshot
// ---------------------------------------------------------------------------
App52Snapshot_t   g_app52_snapshot       = {
    false,false,false,
    ANALOG_MODE_AUTO, 0, 0, 0, 0, 0, 0,
    false, false, false, 0
};
SemaphoreHandle_t g_app52_snapshot_mutex = NULL;

#define SEM_TICKS pdMS_TO_TICKS(10)

// ---------------------------------------------------------------------------
// Persistent conditioning state
// ---------------------------------------------------------------------------
static int     s_med_buf[MED_WINDOW] = {0};
static uint8_t s_med_idx             = 0;
static int     s_weighted_prev       = 0;
static int     s_ramp_pwm            = 0;
static bool    s_alert_latch         = false;

// ---------------------------------------------------------------------------
// Step 1 – Saturation
// ---------------------------------------------------------------------------
static int saturate(int v) {
    if (v < PWM_MIN) return PWM_MIN;
    if (v > PWM_MAX) return PWM_MAX;
    return v;
}

// ---------------------------------------------------------------------------
// Step 2 – Median filter  (3-sample, sort-free)
// ---------------------------------------------------------------------------
static int push_median(int v) {
    s_med_buf[s_med_idx] = v;
    s_med_idx = (uint8_t)((s_med_idx + 1) % MED_WINDOW);
    int a = s_med_buf[0], b = s_med_buf[1], c = s_med_buf[2];
    if ((a <= b && b <= c) || (c <= b && b <= a)) return b;
    if ((b <= a && a <= c) || (c <= a && a <= b)) return a;
    return c;
}

// ---------------------------------------------------------------------------
// Step 3 – Weighted average  (70% new + 30% previous)
// ---------------------------------------------------------------------------
static int weighted_avg(int new_val) {
    int result = (WEIGHTED_ALPHA * new_val +
                  (100 - WEIGHTED_ALPHA) * s_weighted_prev) / 100;
    s_weighted_prev = result;
    return result;
}

// ---------------------------------------------------------------------------
// Step 4 – Ramp limiter  (max ±RAMP_STEP_PWM per tick)
// ---------------------------------------------------------------------------
static int advance_ramp(int target) {
    int delta = target - s_ramp_pwm;
    if (delta >  RAMP_STEP_PWM) delta =  RAMP_STEP_PWM;
    if (delta < -RAMP_STEP_PWM) delta = -RAMP_STEP_PWM;
    s_ramp_pwm += delta;
    if (s_ramp_pwm < PWM_MIN) s_ramp_pwm = PWM_MIN;
    if (s_ramp_pwm > PWM_MAX) s_ramp_pwm = PWM_MAX;
    return s_ramp_pwm;
}

// ---------------------------------------------------------------------------
// Alert – two-threshold hysteresis
// ---------------------------------------------------------------------------
static bool update_alert(int ramped) {
    if (!s_alert_latch && ramped >  ALERT_HIGH_PWM) s_alert_latch = true;
    if ( s_alert_latch && ramped <  ALERT_LOW_PWM)  s_alert_latch = false;
    return s_alert_latch;
}

// ---------------------------------------------------------------------------
// LEDs
//   LED 0 RED    – mirrors relay state
//   LED 1 GREEN  – no alert (healthy)
//   LED 2 YELLOW – alert active
// ---------------------------------------------------------------------------
static void set_leds(bool relay_on, bool alert_on) {
    relay_on ? dd_led_turn_on()    : dd_led_turn_off();
    alert_on ? dd_led_1_turn_off() : dd_led_1_turn_on();
    alert_on ? dd_led_2_turn_on()  : dd_led_2_turn_off();
}

// ---------------------------------------------------------------------------
// Export snapshot – single atomic block
// ---------------------------------------------------------------------------
static void export_snapshot(const App52UserCmd_t *cmd,
                             bool bin_state,  bool bin_pending,
                             int  pot_raw,    int  pwm_input,
                             int  pwm_sat,    int  pwm_med,
                             int  pwm_wgt,    int  pwm_rmp,
                             bool alert) {
    if (xSemaphoreTake(g_app52_snapshot_mutex, SEM_TICKS) != pdTRUE) return;

    g_app52_snapshot.bin_requested  = cmd->bin_on;
    g_app52_snapshot.bin_pending    = bin_pending;
    g_app52_snapshot.bin_state      = bin_state;
    g_app52_snapshot.analog_mode    = cmd->analog_mode;
    g_app52_snapshot.pot_raw        = pot_raw;
    g_app52_snapshot.pwm_raw        = pwm_input;
    g_app52_snapshot.pwm_saturated  = pwm_sat;
    g_app52_snapshot.pwm_median     = pwm_med;
    g_app52_snapshot.pwm_weighted   = pwm_wgt;
    g_app52_snapshot.pwm_ramped     = pwm_rmp;
    g_app52_snapshot.motor_alert    = alert;
    g_app52_snapshot.at_limit_max   = cmd->at_limit_max;
    g_app52_snapshot.at_limit_min   = cmd->at_limit_min;
    g_app52_snapshot.uptime_ms      =
        (uint32_t)((uint32_t)xTaskGetTickCount() * portTICK_PERIOD_MS);

    xSemaphoreGive(g_app52_snapshot_mutex);
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
void task2_setup() {
    g_app52_snapshot_mutex = xSemaphoreCreateMutex();
    pinMode(PIN_POTENTIOMETER, INPUT);
}

// ---------------------------------------------------------------------------
// Task entry point
// ---------------------------------------------------------------------------
void task2_run(void *pvParameters) {
    (void)pvParameters;
    TickType_t next_tick = xTaskGetTickCount();

    for (;;) {
        App52UserCmd_t cmd = task1_get_cmd();

        // ---- Read pot (always, shown in report even in MANUAL mode) --------
        int pot_raw = analogRead(PIN_POTENTIOMETER);
        int pot_pwm = (int)map((long)pot_raw, 0L, 1023L,
                               (long)PWM_MIN, (long)PWM_MAX);

        // ---- Select pipeline input -----------------------------------------
        int pwm_input = (cmd.analog_mode == ANALOG_MODE_AUTO)
                        ? pot_pwm
                        : cmd.pwm_manual;

        // ---- Signal conditioning pipeline ----------------------------------
        int pwm_sat = saturate(pwm_input);
        int pwm_med = push_median(pwm_sat);
        int pwm_wgt = weighted_avg(pwm_med);
        int pwm_rmp = advance_ramp(pwm_wgt);

        // ---- Drive motor ---------------------------------------------------
        act_analog_tick(pwm_rmp);

        // ---- Drive relay + relay LED (via act_binary) ----------------------
        act_binary_request(cmd.bin_on ? 1 : 0);
        act_binary_tick();
        bool bin_state   = (act_binary_get_state()   == 1);
        bool bin_pending = (act_binary_get_pending()  != act_binary_get_state());

        // ---- Alert ---------------------------------------------------------
        bool alert = update_alert(pwm_rmp);

        // ---- Status LEDs ---------------------------------------------------
        set_leds(bin_state, alert);

        // ---- Publish snapshot ---------------------------------------------
        export_snapshot(&cmd,
                        bin_state, bin_pending,
                        pot_raw, pwm_input,
                        pwm_sat, pwm_med, pwm_wgt, pwm_rmp,
                        alert);

        vTaskDelayUntil(&next_tick, pdMS_TO_TICKS(COND_PERIOD_MS));
    }
}