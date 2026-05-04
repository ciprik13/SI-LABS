#ifndef APP_LAB_6_2_TASK_CONFIG_H
#define APP_LAB_6_2_TASK_CONFIG_H

#include <Arduino_FreeRTOS.h>
#include <semphr.h>

// ===========================================================================
// task_config.h – Shared types and constants for Lab 6.2
//
// PID temperature control (Variant A)
//
// Sensor   : DHT22 on pin 2 (ed_dht driver, throttled to 1 s)
// Actuator : Relay on pin 6 (digital ON/OFF; PID output drives duty cycle
//            via time-proportional relay switching over a 2 s window)
// LEDs:
//   RED    (pin  9) – blinks 250 ms when |error| > ALARM_THRESHOLD
//   GREEN  (pin 12) – solid ON when output = 0 (no heating needed)
//   YELLOW (pin 11) – solid ON when output > 0 (heater active)
//
// SetPoint / PID commands (serial, case-insensitive):
//   SP <value>    – set setpoint (°C)
//   SP+           – increment setpoint by SP_STEP
//   SP-           – decrement setpoint by SP_STEP
//   KP <value>    – set proportional gain  (float, e.g. KP 2.5)
//   KI <value>    – set integral gain      (float)
//   KD <value>    – set derivative gain    (float)
//   MODE AUTO     – enable PID control (default)
//   MODE MANUAL   – disable PID; use DUTY <pct> to set output directly
//   DUTY <0-100>  – manual duty cycle % (only in MANUAL mode)
//   STATUS        – force immediate serial report
//   HELP          – print command list
//
// Bonus – Manual / Auto mode switch:
//   In MANUAL mode the PID integrator is frozen and output is set directly
//   via the DUTY command. Switching back to AUTO performs bumpless transfer
//   by pre-loading the integrator with the last manual duty value.
//
// Arduino Serial Plotter output (task_3 every 500 ms):
//   SetPoint:<sp>  Value:<temp>  Output:<duty_pct>
// ===========================================================================

// --- Task periods -----------------------------------------------------------
#define CMD_PERIOD_MS           20     // task_1: command decode
#define COND_PERIOD_MS         100     // task_2: PID control tick (10 Hz)
#define REPORT_PERIOD_MS       500     // task_3: LCD + serial report
#define SERIAL_HEARTBEAT_MS  10000     // task_3: force-reprint interval

// --- Hardware pins ----------------------------------------------------------
#define PIN_RELAY          6    // Heater relay (time-proportional PWM)
#define PIN_LED_RED        9    // RED    – blinks on alarm deviation
#define PIN_LED_GREEN     12    // GREEN  – output = 0 (comfortable)
#define PIN_LED_YELLOW    11    // YELLOW – output > 0 (heater active)

// --- SetPoint defaults and limits -------------------------------------------
#define SP_DEFAULT        22    // °C
#define SP_MIN             0    // °C
#define SP_MAX            50    // °C
#define SP_STEP            1    // °C per SP+ / SP-

// --- PID defaults -----------------------------------------------------------
// Gains tuned for slow thermal system (DHT22 + resistor heater, ~2 s lag):
//   Kp = 15.0  → moderate proportional response
//   Ki =  0.5  → slow integral wind-up removal
//   Kd =  2.0  → light derivative damping
#define KP_DEFAULT        15.0f
#define KI_DEFAULT         0.5f
#define KD_DEFAULT         2.0f

// --- PID output limits (duty cycle %) ---------------------------------------
#define PID_OUT_MIN        0.0f   // 0 % duty  (heater fully off)
#define PID_OUT_MAX      100.0f   // 100 % duty (heater fully on)

// --- Time-proportional relay window -----------------------------------------
// The relay is cycled over this window using the PID duty output.
// e.g. duty=60% over 2000 ms → relay ON for 1200 ms, OFF for 800 ms.
#define RELAY_WINDOW_MS   2000

// --- Alarm threshold --------------------------------------------------------
// RED LED blinks when |error| > ALARM_THRESHOLD (°C)
#define ALARM_THRESHOLD    5.0f

// --- Bonus: blink period ----------------------------------------------------
#define BLINK_PERIOD_MS   250

// ---------------------------------------------------------------------------
// User command – produced by task_1, consumed by task_2
// ---------------------------------------------------------------------------
typedef struct {
    // SetPoint
    int   sp;               // current setpoint (°C)

    // PID gains
    float kp;
    float ki;
    float kd;

    // Mode
    bool  manual_mode;      // true = MANUAL, false = AUTO
    float manual_duty;      // 0–100 %, only used in MANUAL mode

    // Flags
    bool  force_report;     // STATUS command
} App62UserCmd_t;

// ---------------------------------------------------------------------------
// Snapshot – produced by task_2, consumed by task_3
// ---------------------------------------------------------------------------
typedef struct {
    // Sensor
    int   temp_raw;         // temperature × 10  (0.1 °C resolution)
    float temp_f;           // float °C
    int   humidity;         // integer %RH

    // Control
    int   sp;               // active setpoint (°C)
    float kp;
    float ki;
    float kd;
    float error;            // sp - temp_f
    float pid_p;            // proportional term
    float pid_i;            // integral accumulator
    float pid_d;            // derivative term
    float pid_out;          // clamped output 0–100 %

    // Actuator
    bool  relay_on;         // current relay state (within window)
    bool  manual_mode;      // true = MANUAL
    float manual_duty;      // manual override duty %

    // Bonus alarm
    bool  alarm;            // |error| > ALARM_THRESHOLD

    bool  force_report;
    uint32_t uptime_ms;
} App62Snapshot_t;

extern App62Snapshot_t   g_app62_snapshot;
extern SemaphoreHandle_t g_app62_snapshot_mutex;
extern SemaphoreHandle_t g_app62_io_mutex;

#endif // APP_LAB_6_2_TASK_CONFIG_H