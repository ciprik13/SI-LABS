#ifndef APP_LAB_5_1_TASK_CONFIG_H
#define APP_LAB_5_1_TASK_CONFIG_H

#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <stdint.h>
#include <stdbool.h>

// ===========================================================================
// Hardware pin definitions
// ===========================================================================
#define BINARY_ACT_PIN      13   // RED LED – simulates relay / lamp
#define ANALOG_ACT_PIN       9   // PWM output – simulates dimmer (0-255)

// ===========================================================================
// Binary actuator – debounce & saturation
// ===========================================================================
#define BIN_DEBOUNCE_SAMPLES    3   // 3 × 50 ms = 150 ms min persistence
#define BIN_SAT_ON              1   // saturated ON value
#define BIN_SAT_OFF             0   // saturated OFF value

// ===========================================================================
// Analog actuator – operating range (percent)
// ===========================================================================
#define ANLG_SAT_LOW            0   // % minimum
#define ANLG_SAT_HIGH         100   // % maximum
#define ANLG_ALERT_THRESHOLD   80   // % – alert if level exceeds this
#define ANLG_ALERT_HYST        70   // % – alert clears below this (hysteresis)
#define ANLG_DEBOUNCE_SAMPLES   4   // 4 × 75 ms = 300 ms min persistence

// ===========================================================================
// Command source identifier
// ===========================================================================
#define CMD_SRC_SERIAL   's'

// ===========================================================================
// Shared command state
// Written by task51_cmd_input, read by task51_signal_cond.
// Protected by g51_cmd_mutex.
// ===========================================================================
typedef struct {
    int  raw_cmd;        // 1 = ON requested, 0 = OFF requested, -1 = none
    char source;         // CMD_SRC_SERIAL or CMD_SRC_KEYPAD
    bool invalid_cmd;    // true if last command string was unrecognised
} CmdState51_t;

extern CmdState51_t      g51_cmd;
extern SemaphoreHandle_t g51_cmd_mutex;

// ===========================================================================
// Shared binary actuator state
// Written by task51_signal_cond & task51_binary_ctrl,
// read by task51_display.
// Protected by g51_bin_mutex.
// ===========================================================================
typedef struct {
    int  requested;      // last sanitised command (0 or 1)
    int  pending;        // candidate value being debounced
    int  bounce_count;   // consecutive confirmations of pending
    int  committed;      // committed (debounced) state
    bool actuator_on;    // actual pin output state
    bool in_error;       // set if hardware write fails (stub: always false)
} BinActState51_t;

extern BinActState51_t   g51_bin;
extern SemaphoreHandle_t g51_bin_mutex;

// ===========================================================================
// Shared analog actuator state
// Written by task51_analog_ctrl, read by task51_display.
// Protected by g51_anlg_mutex.
// ===========================================================================
typedef struct {
    int  raw_pot;        // raw potentiometer reading (0–100 %)
    int  saturated;      // after saturation clamping
    int  pwm_value;      // 0–255 written to ANALOG_ACT_PIN
    int  level_pct;      // 0–100 % (display friendly)
    bool alert_active;   // committed over-threshold alert
    bool pending_alert;  // candidate alert being debounced
    int  alert_bounce;   // consecutive confirmations of pending_alert
} AnlgActState51_t;

extern AnlgActState51_t  g51_anlg;
extern SemaphoreHandle_t g51_anlg_mutex;

// ===========================================================================
// Initialise all mutexes (call once before xTaskCreate)
// ===========================================================================
void task51_init();

#endif // APP_LAB_5_1_TASK_CONFIG_H