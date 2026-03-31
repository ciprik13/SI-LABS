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
// actuator_ctrl – Signal conditioning and actuator drive loop (25 ms, pri 2)
// ===========================================================================

// ---------------------------------------------------------------------------
// Shared snapshot  (read by task_3 via g_app5_snapshot_mutex)
// ---------------------------------------------------------------------------
App5Snapshot_t    g_app5_snapshot       = { false,false,false,
                                            ANALOG_MODE_AUTO,0,0,0,false };
SemaphoreHandle_t g_app5_snapshot_mutex = NULL;

#define SEM_TICKS pdMS_TO_TICKS(10)

static bool s_alert_latch = false;

// ---------------------------------------------------------------------------
// App5CondState_t – renamed to avoid ODR conflict with app_lab_4_1
// ---------------------------------------------------------------------------
typedef struct {
    int  pot_angle;
    int  pwm_target;
    int  pwm_output;
    bool relay_on;
    bool relay_pending;
    bool alert_active;
    AnalogControlMode_t ctrl_mode;
} App5CondState_t;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static int angle_to_duty(int deg) {
    long duty = map((long)deg, -135L, 135L,
                    (long)ANALOG_PWM_MIN, (long)ANALOG_PWM_MAX);
    if (duty < ANALOG_PWM_MIN) duty = ANALOG_PWM_MIN;
    if (duty > ANALOG_PWM_MAX) duty = ANALOG_PWM_MAX;
    return (int)duty;
}

static void step_relay(bool requested) {
    act_binary_request(requested ? 1 : 0);
    act_binary_tick();
}

static void step_motor(AnalogControlMode_t mode, int manual_level,
                        int pot_angle, int *out_target, int *out_output) {
    *out_target = (mode == ANALOG_MODE_AUTO)
                  ? angle_to_duty(pot_angle)
                  : manual_level;
    act_analog_tick(*out_target);
    *out_output = act_analog_get_pwm();
}

static bool update_alert(int duty) {
    if (!s_alert_latch && duty >  ANALOG_ALERT_HIGH) s_alert_latch = true;
    if ( s_alert_latch && duty <  ANALOG_ALERT_LOW)  s_alert_latch = false;
    return s_alert_latch;
}

static App5CondState_t collect_cycle(const App5UserCmd_t *intent) {
    App5CondState_t cs;

    dd_sns_angle_loop();
    cs.pot_angle  = dd_sns_angle_get_value();
    cs.ctrl_mode  = intent->analog_mode;

    step_relay(intent->bin_requested);
    step_motor(cs.ctrl_mode, intent->manual_pwm,
               cs.pot_angle, &cs.pwm_target, &cs.pwm_output);

    cs.relay_on      = (act_binary_get_state()   == 1);
    cs.relay_pending = (act_binary_get_pending()  != act_binary_get_state());
    cs.alert_active  = update_alert(cs.pwm_output);

    return cs;
}

static void export_snapshot(const App5CondState_t *cs, bool bin_requested) {
    if (xSemaphoreTake(g_app5_snapshot_mutex, SEM_TICKS) != pdTRUE) return;

    g_app5_snapshot.bin_requested        = bin_requested;
    g_app5_snapshot.bin_pending          = cs->relay_pending;
    g_app5_snapshot.bin_state            = cs->relay_on;
    g_app5_snapshot.analog_mode          = cs->ctrl_mode;
    g_app5_snapshot.angle_deg            = cs->pot_angle;
    g_app5_snapshot.analog_requested_pwm = cs->pwm_target;
    g_app5_snapshot.analog_applied_pwm   = cs->pwm_output;
    g_app5_snapshot.analog_alert         = cs->alert_active;

    xSemaphoreGive(g_app5_snapshot_mutex);
}

static void set_indicators(bool relay_on, bool alert_on) {
    relay_on ? dd_led_turn_on()    : dd_led_turn_off();
    alert_on ? dd_led_1_turn_off() : dd_led_1_turn_on();
    alert_on ? dd_led_2_turn_on()  : dd_led_2_turn_off();
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
void actuator_ctrl_setup() {
    g_app5_snapshot_mutex = xSemaphoreCreateMutex();
}

void actuator_ctrl_run(void *pvParameters) {
    (void)pvParameters;
    TickType_t next_tick = xTaskGetTickCount();

    for (;;) {
        App5UserCmd_t   intent = input_handler_get_cmd();
        App5CondState_t cs     = collect_cycle(&intent);

        export_snapshot(&cs, intent.bin_requested);
        set_indicators(cs.relay_on, cs.alert_active);

        vTaskDelayUntil(&next_tick, pdMS_TO_TICKS(ACTUATOR_COND_PERIOD_MS));
    }
}