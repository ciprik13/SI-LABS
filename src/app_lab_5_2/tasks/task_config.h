#ifndef APP_LAB_5_2_TASK_CONFIG_H
#define APP_LAB_5_2_TASK_CONFIG_H

#include <Arduino_FreeRTOS.h>
#include <semphr.h>

// ===========================================================================
// task_config.h – Shared types and constants for Lab 5.2
//
// Dual-actuator system:
//   Binary  : relay controlled via ON/OFF commands
//             RED LED (pin 9) lights up when relay is ON
//   Analog  : DC motor via L298N, speed PWM 0-255
//
// LED assignment:
//   RED    (pin  9) – relay ON indicator
//   GREEN  (pin 12) – system OK / no alert
//   YELLOW (pin 11) – over-speed alert active
//
// Accepted commands (case-insensitive):
//   ON              – engage relay  (RED LED on)
//   OFF             – disengage relay (RED LED off)
//   AUTO            – motor speed tracks potentiometer
//   PWM <0..255>    – motor speed manual fixed value
//   INC             – increment manual PWM by SPEED_STEP (bonus)
//   DEC             – decrement manual PWM by SPEED_STEP (bonus)
//   HELP            – print command list
// ===========================================================================

// --- Task periods -----------------------------------------------------------
#define CMD_PERIOD_MS             20    // task_1: command input
#define COND_PERIOD_MS            25    // task_2: conditioning + drive
#define REPORT_PERIOD_MS         500    // task_3: display / serial
#define SERIAL_HEARTBEAT_MS    10000    // task_3: force-reprint interval

// --- Debounce ---------------------------------------------------------------
#define BIN_DEBOUNCE_SAMPLES       3    // 3 × 25 ms = 75 ms persistence

// --- Hardware pins ----------------------------------------------------------
#define PIN_RELAY         13    // Binary actuator – relay coil
#define PIN_LED_BIN_ON     9    // RED    – relay ON indicator
#define PIN_LED_OK        12    // GREEN  – no alert
#define PIN_LED_ALERT     11    // YELLOW – alert active

#define PIN_MOTOR_ENA     10    // L298N ENA – PWM speed
#define PIN_MOTOR_IN1      8    // L298N IN1
#define PIN_MOTOR_IN2      7    // L298N IN2

#define PIN_POTENTIOMETER A0    // Analog input for AUTO mode

// --- Analog actuator limits -------------------------------------------------
#define PWM_MIN             0
#define PWM_MAX           255
#define SPEED_STEP         25   // INC/DEC step size (bonus)

// --- Alert thresholds (PWM values) ------------------------------------------
#define ALERT_HIGH_PWM    220   // alert ON  edge  (~86%)
#define ALERT_LOW_PWM     200   // alert OFF edge  (hysteresis)

// --- Signal conditioning parameters ----------------------------------------
#define MED_WINDOW          3   // median filter window (samples)
#define WEIGHTED_ALPHA     70   // weighted avg: 70% new + 30% old
#define RAMP_STEP_PWM      10   // max PWM change per COND tick (soft ramp)

// --- Analog control mode ----------------------------------------------------
typedef enum {
    ANALOG_MODE_AUTO   = 0,    // speed tracks potentiometer
    ANALOG_MODE_MANUAL = 1     // speed fixed by PWM command
} AnalogMode_t;

// --- User command – produced by task_1, consumed by task_2 ----------------
typedef struct {
    bool         bin_on;         // relay requested state
    AnalogMode_t analog_mode;    // AUTO or MANUAL
    int          pwm_manual;     // valid when mode == MANUAL (0-255)
    bool         at_limit_max;   // INC/PWM hit 255 (bonus flag)
    bool         at_limit_min;   // DEC/PWM hit 0   (bonus flag)
} App52UserCmd_t;

// --- Snapshot – produced by task_2, consumed by task_3 -------------------
typedef struct {
    // Binary actuator
    bool         bin_requested;
    bool         bin_pending;
    bool         bin_state;

    // Analog actuator – full conditioning pipeline
    AnalogMode_t analog_mode;
    int          pot_raw;         // raw ADC value 0-1023
    int          pwm_raw;         // pipeline input (pot-mapped or manual)
    int          pwm_saturated;   // after saturation
    int          pwm_median;      // after median filter
    int          pwm_weighted;    // after weighted average
    int          pwm_ramped;      // after ramp = applied to hardware

    // Status
    bool         motor_alert;
    bool         at_limit_max;
    bool         at_limit_min;

    uint32_t     uptime_ms;
} App52Snapshot_t;

extern App52Snapshot_t   g_app52_snapshot;
extern SemaphoreHandle_t g_app52_snapshot_mutex;

#endif // APP_LAB_5_2_TASK_CONFIG_H