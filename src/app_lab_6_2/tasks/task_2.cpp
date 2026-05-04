#include "task_2.h"
#include "task_1.h"
#include "task_config.h"
#include "ed_dht/ed_dht.h"
#include "dd_led/dd_led.h"
#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <Arduino.h>
#include <stdio.h>
#include <math.h>

// ===========================================================================
// task_2 – DHT acquisition + PID control  (100 ms period, priority 2)
//
// PID algorithm (positional form, heating direction):
//   e(t)   = SP - temp
//   P      = Kp * e
//   I     += Ki * e * dt      (anti-windup: I clamped to output limits)
//   D      = Kd * (e - e_prev) / dt
//   output = clamp(P + I + D, 0, 100)
//
// Time-proportional relay:
//   The relay has no analog drive, so PID duty (0–100 %) is realised by
//   toggling the relay within a fixed RELAY_WINDOW_MS window.
//   ON_time = (output / 100) * RELAY_WINDOW_MS
//   Each 100 ms tick advances a window counter; the relay state is set once
//   at the start of each new window based on the latest output.
//
// Bonus – Manual / Auto mode:
//   MANUAL: PID integrator is frozen; relay driven directly by manual_duty.
//   AUTO (return from MANUAL): integrator pre-loaded with manual_duty so
//   the first PID output equals the last manual output (bumpless transfer).
// ===========================================================================

// ---------------------------------------------------------------------------
// Shared snapshot
// ---------------------------------------------------------------------------
App62Snapshot_t   g_app62_snapshot       = {
    0, 0.0f, 0,
    SP_DEFAULT, KP_DEFAULT, KI_DEFAULT, KD_DEFAULT,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    false, false, 0.0f,
    false, false, 0
};
SemaphoreHandle_t g_app62_snapshot_mutex = NULL;

#define SEM_TICKS pdMS_TO_TICKS(10)

// ---------------------------------------------------------------------------
// PID state
// ---------------------------------------------------------------------------
static float s_integrator  = 0.0f;   // integral accumulator
static float s_prev_error  = 0.0f;   // previous error for derivative
static bool  s_was_manual  = false;  // tracks last mode for bumpless transfer

// dt in seconds for the 100 ms tick
static const float DT = (float)COND_PERIOD_MS / 1000.0f;

// ---------------------------------------------------------------------------
// Time-proportional relay state
// ---------------------------------------------------------------------------
static uint32_t s_window_start_ms = 0;    // millis() at start of current window
static float    s_window_duty     = 0.0f; // duty locked in at window start

// ---------------------------------------------------------------------------
// PID compute
// Returns output 0–100 %.
// Anti-windup: integral clamped so that I alone stays within output limits.
// ---------------------------------------------------------------------------
static float pid_compute(float sp, float temp, float kp, float ki, float kd) {
    float error = sp - temp;

    // Proportional
    float p_term = kp * error;

    // Integral with anti-windup clamping
    s_integrator += ki * error * DT;
    if (s_integrator > PID_OUT_MAX) s_integrator = PID_OUT_MAX;
    if (s_integrator < PID_OUT_MIN) s_integrator = PID_OUT_MIN;
    float i_term = s_integrator;

    // Derivative (on error, not on measurement – avoids derivative kick on SP step)
    float d_term = kd * (error - s_prev_error) / DT;
    s_prev_error = error;

    // Sum and clamp
    float out = p_term + i_term + d_term;
    if (out > PID_OUT_MAX) out = PID_OUT_MAX;
    if (out < PID_OUT_MIN) out = PID_OUT_MIN;

    return out;
}

// ---------------------------------------------------------------------------
// Time-proportional relay tick
// Called every COND_PERIOD_MS. Returns true if relay should be ON this tick.
// ---------------------------------------------------------------------------
static bool relay_tick(float duty_pct) {
    uint32_t now     = (uint32_t)((uint32_t)xTaskGetTickCount() * portTICK_PERIOD_MS);
    uint32_t elapsed = now - s_window_start_ms;

    // Start a new window
    if (elapsed >= (uint32_t)RELAY_WINDOW_MS) {
        s_window_start_ms = now;
        s_window_duty     = duty_pct;
        elapsed           = 0;
    }

    // ON for the first (duty/100 * RELAY_WINDOW_MS) ms of the window
    uint32_t on_time = (uint32_t)(s_window_duty * (float)RELAY_WINDOW_MS / 100.0f);
    return (elapsed < on_time);
}

// ---------------------------------------------------------------------------
// LED update
//   RED    – blinks at BLINK_PERIOD_MS when alarm active
//   YELLOW – solid ON when output > 0
//   GREEN  – solid ON when output == 0
// ---------------------------------------------------------------------------
static TickType_t s_blink_last  = 0;
static bool       s_blink_state = false;

static void update_leds(float output, bool alarm) {
    // YELLOW: heater producing some output
    (output > 0.5f) ? dd_led_2_turn_on() : dd_led_2_turn_off();

    // GREEN: no output needed (comfortable)
    (output < 0.5f) ? dd_led_1_turn_on() : dd_led_1_turn_off();

    // RED: blink on alarm deviation
    if (alarm) {
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
static void export_snapshot(int temp_raw, float temp_f, int humidity,
                             int sp, float kp, float ki, float kd,
                             float error, float p, float i_acc, float d,
                             float pid_out, bool relay_on,
                             bool manual_mode, float manual_duty,
                             bool alarm, bool force_report) {
    if (xSemaphoreTake(g_app62_snapshot_mutex, SEM_TICKS) != pdTRUE) return;

    g_app62_snapshot.temp_raw    = temp_raw;
    g_app62_snapshot.temp_f      = temp_f;
    g_app62_snapshot.humidity    = humidity;
    g_app62_snapshot.sp          = sp;
    g_app62_snapshot.kp          = kp;
    g_app62_snapshot.ki          = ki;
    g_app62_snapshot.kd          = kd;
    g_app62_snapshot.error       = error;
    g_app62_snapshot.pid_p       = p;
    g_app62_snapshot.pid_i       = i_acc;
    g_app62_snapshot.pid_d       = d;
    g_app62_snapshot.pid_out     = pid_out;
    g_app62_snapshot.relay_on    = relay_on;
    g_app62_snapshot.manual_mode = manual_mode;
    g_app62_snapshot.manual_duty = manual_duty;
    g_app62_snapshot.alarm       = alarm;
    g_app62_snapshot.force_report = force_report;
    g_app62_snapshot.uptime_ms   =
        (uint32_t)((uint32_t)xTaskGetTickCount() * portTICK_PERIOD_MS);

    xSemaphoreGive(g_app62_snapshot_mutex);
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
void task2_62_setup() {
    g_app62_snapshot_mutex = xSemaphoreCreateMutex();
    ed_dht_setup();
    s_window_start_ms = (uint32_t)((uint32_t)xTaskGetTickCount() * portTICK_PERIOD_MS);
}

// ---------------------------------------------------------------------------
// Task entry point
// ---------------------------------------------------------------------------
void task2_62_run(void *pvParameters) {
    (void)pvParameters;
    TickType_t next_tick = xTaskGetTickCount();

    for (;;) {
        App62UserCmd_t cmd = task1_62_get_cmd();

        // --- Acquire sensor -------------------------------------------------
        ed_dht_loop();
        int   temp_raw = ed_dht_get_raw();
        float temp_f   = (float)temp_raw / 10.0f;
        int   humidity = ed_dht_get_humidity();

        // --- Plausibility gate: skip until sensor gives a non-zero read ------
        static bool s_sensor_ready = false;
        bool in_range = (temp_raw >= -400 && temp_raw <= 800 &&
                         humidity >=    0 && humidity <=  100);
        bool non_zero = (temp_raw != 0 || humidity != 0);
        if (in_range && non_zero) s_sensor_ready = true;
        if (!s_sensor_ready) {
            vTaskDelayUntil(&next_tick, pdMS_TO_TICKS(COND_PERIOD_MS));
            continue;
        }

        // --- PID or MANUAL --------------------------------------------------
        float error   = (float)cmd.sp - temp_f;
        float p_term  = 0.0f;
        float d_term  = 0.0f;
        float pid_out = 0.0f;

        if (cmd.manual_mode) {
            // MANUAL: freeze integrator, drive relay with manual duty directly
            s_prev_error = error;   // keep derivative state current
            s_was_manual = true;
            pid_out      = cmd.manual_duty;
            p_term       = 0.0f;
            d_term       = 0.0f;
        } else {
            // AUTO: bumpless transfer on return from MANUAL
            if (s_was_manual) {
                // Pre-load integrator so first output = last manual duty
                s_integrator = cmd.manual_duty;
                s_prev_error = error;
                s_was_manual = false;
            }
            pid_out = pid_compute(
                (float)cmd.sp, temp_f,
                cmd.kp, cmd.ki, cmd.kd
            );
            p_term  = cmd.kp * error;
            d_term  = cmd.kd * (error - s_prev_error) / DT;
            // Note: s_prev_error already updated inside pid_compute
        }

        // --- Time-proportional relay ----------------------------------------
        bool relay_on = relay_tick(pid_out);
        digitalWrite(PIN_RELAY, relay_on ? HIGH : LOW);

        // --- Alarm ----------------------------------------------------------
        bool alarm = (fabsf(error) > ALARM_THRESHOLD);

        // --- Status LEDs ----------------------------------------------------
        update_leds(pid_out, alarm);

        // --- Publish snapshot -----------------------------------------------
        export_snapshot(
            temp_raw, temp_f, humidity,
            cmd.sp, cmd.kp, cmd.ki, cmd.kd,
            error, p_term, s_integrator, d_term, pid_out,
            relay_on, cmd.manual_mode, cmd.manual_duty,
            alarm, cmd.force_report
        );

        vTaskDelayUntil(&next_tick, pdMS_TO_TICKS(COND_PERIOD_MS));
    }
}