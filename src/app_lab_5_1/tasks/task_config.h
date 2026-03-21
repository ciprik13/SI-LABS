#ifndef APP_LAB_5_1_TASK_CONFIG_H
#define APP_LAB_5_1_TASK_CONFIG_H

#include <Arduino_FreeRTOS.h>
#include <semphr.h>

// ===========================================================================
// task_config.h – Shared types and constants for Lab 5.1
//
// Internal actuator state is hidden inside act_binary.cpp / act_analog.cpp.
// Tasks access actuators exclusively through the public API functions.
// ===========================================================================

// --- Task periods -----------------------------------------------------------
#define ACTUATOR_CMD_PERIOD_MS          20    // task_1: command input
#define ACTUATOR_COND_PERIOD_MS         25    // task_2: conditioning + apply
#define ACTUATOR_REPORT_PERIOD_MS      500    // task_3: display tick
#define ACTUATOR_SERIAL_HEARTBEAT_MS 10000    // task_3: force-print interval

// --- Debounce ---------------------------------------------------------------
#define BIN_CMD_PERSISTENCE_SAMPLES      3    // 3 × 20 ms = 60 ms min persist

// --- Hardware pins ----------------------------------------------------------
#define PIN_LED_BIN_ON    9    // RED    – motor ON indicator (mirrors binary actuator state)
#define PIN_LED_OK       12    // GREEN  – system OK / no analog alert
#define PIN_LED_ALERT    11    // YELLOW – analog alert active

#define PIN_MOTOR_ENA    10    // L298N ENA – PWM speed
#define PIN_MOTOR_IN1     8    // L298N IN1 – direction A
#define PIN_MOTOR_IN2     7    // L298N IN2 – direction B

// --- Analog actuator thresholds (PWM 0-255) ---------------------------------
#define ANALOG_PWM_MIN     0
#define ANALOG_PWM_MAX   255
#define ANALOG_ALERT_HIGH  220   // PWM alert ON  edge
#define ANALOG_ALERT_LOW   200   // PWM alert OFF edge (hysteresis)

// --- Analog control mode ----------------------------------------------------
typedef enum {
    ANALOG_MODE_AUTO   = 0,   // level follows potentiometer
    ANALOG_MODE_MANUAL = 1    // level set by "PWM <val>" Serial command
} AnalogControlMode_t;

// --- User command – produced by task_1, consumed by task_2 -----------------
typedef struct {
    bool                bin_requested;  // true=ON, false=OFF
    AnalogControlMode_t analog_mode;
    int                 manual_pwm;     // valid when analog_mode == MANUAL
} App5UserCmd_t;

// --- Snapshot – produced by task_2, consumed by task_3 ---------------------
typedef struct {
    bool                bin_requested;
    bool                bin_pending;
    bool                bin_state;
    AnalogControlMode_t analog_mode;
    int                 angle_deg;
    int                 analog_requested_pwm;
    int                 analog_applied_pwm;
    bool                analog_alert;
} App5Snapshot_t;

extern App5Snapshot_t    g_app5_snapshot;
extern SemaphoreHandle_t g_app5_snapshot_mutex;

// --- Init – call once before xTaskCreate ------------------------------------
void task51_init();

#endif // APP_LAB_5_1_TASK_CONFIG_H