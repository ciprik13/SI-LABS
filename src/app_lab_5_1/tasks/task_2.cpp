#include "task_2.h"
#include "task_1.h"
#include "task_config.h"
#include "act_binary/act_binary.h"
#include "act_analog/act_analog.h"
#include "dd_sns_angle/dd_sns_angle.h"
#include "dd_led/dd_led.h"
#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <Arduino.h>

// ===========================================================================
// Shared snapshot – owned here, read by task_3 via g_app5_snapshot_mutex
// ===========================================================================
App5Snapshot_t g_app5_snapshot = {
    false, false, false,
    ANALOG_MODE_AUTO,
    0, 0, 0, false
};
SemaphoreHandle_t g_app5_snapshot_mutex = NULL;

#define MTX_TIMEOUT pdMS_TO_TICKS(10)

void task51_init() {
    g_app5_snapshot_mutex = xSemaphoreCreateMutex();
}

// ---------------------------------------------------------------------------
// Map potentiometer angle (-135..+135 deg) to PWM (0..255)
// ---------------------------------------------------------------------------
static int map_angle_to_pwm(int angle_deg) {
    int pwm = (int)map((long)angle_deg, -135, 135, ANALOG_PWM_MIN, ANALOG_PWM_MAX);
    if (pwm < ANALOG_PWM_MIN) return ANALOG_PWM_MIN;
    if (pwm > ANALOG_PWM_MAX) return ANALOG_PWM_MAX;
    return pwm;
}

// ===========================================================================
// Task 2 – Signal Conditioning + Actuator Control  (25 ms, priority 2)
//
// Each cycle:
//   1. Reads potentiometer angle via dd_sns_angle
//   2. Gets latest user command from task51_task1_get_latest()
//   3. Applies binary actuator (debounce in act_binary_tick)
//   4. Computes PWM from AUTO (pot) or MANUAL (command) mode
//   5. Applies analog actuator (L298N via act_analog_tick)
//   6. Evaluates analog alert with hysteresis
//   7. Writes all state into App5Snapshot_t under one mutex lock
//   8. Updates LED indicators via direct digitalWrite
// ===========================================================================
void task51_conditioning(void *pvParameters) {
    (void) pvParameters;

    TickType_t xLastWakeTime = xTaskGetTickCount();

    for (;;) {
        // --- 1. Read potentiometer ----------------------------------------
        dd_sns_angle_loop();
        int angle_deg = dd_sns_angle_get_value();   // -135..+135

        // --- 2. Read latest user command from task_1 ----------------------
        App5UserCmd_t cmd = task51_task1_get_latest();

        // --- 3. Binary actuator: debounce + apply -------------------------
        act_binary_request(cmd.bin_requested ? 1 : 0);
        act_binary_tick();

        // --- 4. Analog actuator: AUTO or MANUAL ---------------------------
        int requested_pwm = 0;
        if (cmd.analog_mode == ANALOG_MODE_AUTO) {
            requested_pwm = map_angle_to_pwm(angle_deg);
        } else {
            requested_pwm = cmd.manual_pwm;
        }
        act_analog_tick(requested_pwm);
        int applied_pwm = act_analog_get_pwm();

        // --- 5. Analog alert with hysteresis (PWM domain) -----------------
        bool analog_alert   = false;
        bool previous_alert = false;

        if (xSemaphoreTake(g_app5_snapshot_mutex, MTX_TIMEOUT) == pdTRUE) {
            previous_alert = g_app5_snapshot.analog_alert;
            xSemaphoreGive(g_app5_snapshot_mutex);
        }

        if (!previous_alert && applied_pwm > ANALOG_ALERT_HIGH) {
            analog_alert = true;
        } else if (previous_alert && applied_pwm >= ANALOG_ALERT_LOW) {
            analog_alert = true;
        }

        // --- 6. Write snapshot (single lock, all fields at once) ----------
        if (xSemaphoreTake(g_app5_snapshot_mutex, MTX_TIMEOUT) == pdTRUE) {
            g_app5_snapshot.bin_requested        = cmd.bin_requested;
            g_app5_snapshot.bin_pending          = (act_binary_get_pending() !=
                                                    act_binary_get_state());
            g_app5_snapshot.bin_state            = (act_binary_get_state() == 1);
            g_app5_snapshot.analog_mode          = cmd.analog_mode;
            g_app5_snapshot.angle_deg            = angle_deg;
            g_app5_snapshot.analog_requested_pwm = requested_pwm;
            g_app5_snapshot.analog_applied_pwm   = applied_pwm;
            g_app5_snapshot.analog_alert         = analog_alert;
            xSemaphoreGive(g_app5_snapshot_mutex);
        }

        // --- 7. LED indicators via dd_led ---------------------------------
        //   red    = binary actuator ON
        //   green  = no alert
        //   yellow = analog alert active
        if (act_binary_get_state() == 1) dd_led_turn_on();
        else                             dd_led_turn_off();

        if (analog_alert) {
            dd_led_1_turn_off();
            dd_led_2_turn_on();
        } else {
            dd_led_1_turn_on();
            dd_led_2_turn_off();
        }

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(ACTUATOR_COND_PERIOD_MS));
    }
}