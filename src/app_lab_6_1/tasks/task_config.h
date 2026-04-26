#ifndef APP_LAB_6_1_TASK_CONFIG_H
#define APP_LAB_6_1_TASK_CONFIG_H

#include <Arduino_FreeRTOS.h>
#include <semphr.h>

// ===========================================================================
// task_config.h – Shared types and constants for Lab 6.1
//
// ON-OFF hysteresis temperature control (Variant A)
//
// Sensor   : DHT22 on pin 2 (change DHTTYPE in ed_dht.cpp for DHT11)
// Actuator : Relay on pin 6  (heating – relay ON when temp too cold)
// LEDs:
//   RED    (pin  9) – blinks 250 ms when temp deviation > 2×hysteresis
//   GREEN  (pin 12) – ON when relay is OFF (system comfortable)
//   YELLOW (pin 11) – ON when relay is ON  (heater active)
//
// SetPoint commands (serial, case-insensitive):
//   SP <value>   – set setpoint to integer °C
//   SP+          – increment setpoint by SP_STEP
//   SP-          – decrement setpoint by SP_STEP
//   HYST <value> – set hysteresis band (°C, must be >= HYST_MIN)
//   STATUS       – force a serial report immediately
//   HELP         – print command list
//
// ON-OFF hysteresis (heating):
//   relay turns ON  when temp < (SP - HYST/2)   → below lower threshold
//   relay turns OFF when temp > (SP + HYST/2)   → above upper threshold
//
// Bonus:
//   RED LED blinks (250 ms toggle) when |temp - SP| > 2 × HYST
//   (big overshoot / undershoot — extreme deviation)
//
// Arduino Serial Plotter output format (task_3 every 500 ms):
//   SetPoint:<sp> Value:<temp> Output:<0|1>
// ===========================================================================

// --- Task periods -----------------------------------------------------------
#define CMD_PERIOD_MS           20     // task_1: command decode
#define COND_PERIOD_MS          25     // task_2: DHT throttled + control tick
#define REPORT_PERIOD_MS       500     // task_3: LCD + serial report
#define SERIAL_HEARTBEAT_MS  10000     // task_3: force-reprint interval

// --- Hardware pins ----------------------------------------------------------
#define PIN_RELAY          6    // Binary actuator – relay coil (heater)
#define PIN_LED_RED        9    // RED    – blink on extreme deviation
#define PIN_LED_GREEN     12    // GREEN  – comfortable (relay OFF)
#define PIN_LED_YELLOW    11    // YELLOW – heater active (relay ON)

// --- SetPoint defaults and limits -------------------------------------------
#define SP_DEFAULT         22   // °C
#define SP_MIN              0   // °C
#define SP_MAX             50   // °C
#define SP_STEP             1   // °C per SP+ / SP- command

// --- Hysteresis defaults and limits -----------------------------------------
#define HYST_DEFAULT        2   // °C  (ON at 21°C, OFF at 23°C)
#define HYST_MIN            1   // °C  minimum allowed
#define HYST_MAX           10   // °C  maximum allowed

// --- Bonus: extreme-deviation threshold (×hysteresis) ----------------------
#define EXTREME_DEV_FACTOR  2   // |temp - SP| > EXTREME_DEV_FACTOR × HYST

// --- Bonus: RED blink period ------------------------------------------------
#define BLINK_PERIOD_MS   250   // ms half-period for RED LED blink

// --- Control direction ------------------------------------------------------
// HEATING mode: relay ON when temp < lower threshold
#define CONTROL_HEATING     1   // 1 = heating, 0 = cooling

// ---------------------------------------------------------------------------
// User command – produced by task_1, consumed by task_2
// ---------------------------------------------------------------------------
typedef struct {
    bool  sp_changed;     // new SP was issued this cycle
    int   sp;             // current setpoint (°C)
    bool  hyst_changed;   // new HYST was issued this cycle
    int   hyst;           // current hysteresis band (°C)
    bool  force_report;   // STATUS command: force serial print in task_3
} App61UserCmd_t;

// ---------------------------------------------------------------------------
// Snapshot – produced by task_2, consumed by task_3
// ---------------------------------------------------------------------------
typedef struct {
    int      temp_raw;       // temperature × 10  (0.1 °C resolution from DHT)
    int      temp_c;         // integer °C (for control)
    int      humidity;       // integer %RH (displayed, not controlled)

    int      sp;             // active setpoint (°C)
    int      hyst;           // active hysteresis band (°C)

    int      thresh_on;      // SP - HYST/2 (relay turns ON below this)
    int      thresh_off;     // SP + HYST/2 (relay turns OFF above this)

    bool     relay_on;       // committed relay state
    bool     relay_pending;  // debounce in progress

    bool     extreme_dev;    // |temp - SP| > 2 × HYST
    bool     force_report;   // propagated from user cmd

    uint32_t uptime_ms;
} App61Snapshot_t;

extern App61Snapshot_t   g_app61_snapshot;
extern SemaphoreHandle_t g_app61_snapshot_mutex;
extern SemaphoreHandle_t g_app61_io_mutex;

#endif // APP_LAB_6_1_TASK_CONFIG_H
