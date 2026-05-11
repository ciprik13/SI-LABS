#ifndef APP_LAB_7_2_TASK_CONFIG_H
#define APP_LAB_7_2_TASK_CONFIG_H

#include <Arduino_FreeRTOS.h>
#include <semphr.h>

// ===========================================================================
// task_config.h – Shared types and constants for Lab 7.2
//
// Smart Traffic Light FSM – two directions: Est-Vest (EV) and Nord-Sud (NS)
//
// Hardware:
//   Buttons     : PIN_BTN_NS   = 2  (INPUT_PULLUP, active LOW)
//                 PIN_BTN_MODE = 3  (INPUT_PULLUP, active LOW)
//                 BTN_NS short press   → NS request (pedestrian / NS vehicle)
//                 BTN_MODE short press → toggle Night Mode
//   EV LEDs     : GREEN=12, YELLOW=11, RED=9   (Est-Vest direction)
//   NS LEDs     : GREEN=8,  YELLOW=7,  RED=6   (Nord-Sud direction)
//   LCD I2C     : 16x2, address 0x27  (SDA=D20, SCL=D21)
//
// FSM states (Moore FSM – outputs depend only on state):
//   EV_GREEN    – EV green, NS red   (default / priority direction)
//   EV_YELLOW   – EV yellow, NS red  (transition to ALL_RED_1)
//   ALL_RED_1   – both red           (safety gap before NS_GREEN)
//   NS_GREEN    – NS green, EV red   (active when NS request pending)
//   NS_YELLOW   – NS yellow, EV red  (transition to ALL_RED_2)
//   ALL_RED_2   – both red           (safety gap before EV_GREEN)
//   NIGHT_MODE  – both yellow blink 500ms (bonus behaviour)
//
// Transitions:
//   EV_GREEN  → EV_YELLOW  : NS request received AND EV_GREEN_MS elapsed
//   EV_YELLOW → ALL_RED_1  : EV_YELLOW_MS elapsed
//   ALL_RED_1 → NS_GREEN   : ALL_RED_MS elapsed
//   NS_GREEN  → NS_YELLOW  : NS_GREEN_MS elapsed  OR  NS request cleared
//   NS_YELLOW → ALL_RED_2  : NS_YELLOW_MS elapsed
//   ALL_RED_2 → EV_GREEN   : ALL_RED_MS elapsed; NS request cleared
//   ANY       → NIGHT_MODE : BTN_MODE press
//   NIGHT_MODE→ EV_GREEN   : BTN_MODE press
//
// Bonus (10%): Night Mode
//   Both directions blink YELLOW at 500 ms when no traffic is expected.
//   Activated/deactivated using BTN_MODE.
//
// Serial commands (case-insensitive):
//   STATUS  – force immediate serial report
//   HELP    – print command list
//   NIGHT   – toggle night mode
//   REQUEST – simulate NS request (same as button press)
//
// LCD 16x2 layout:
//   Row 0: "EV:GRN  NS:RED  "  (current light states, 3 chars each)
//   Row 1: "T:XXs  [REQ]    "  (time in state, NS request indicator)
//   Night: Row 0: "** NIGHT MODE **"
//          Row 1: "  Yellow blink  "
// ===========================================================================

// --- Task periods -----------------------------------------------------------
#define CMD_PERIOD_MS           20     // task_1: button + command poll (ms)
#define COND_PERIOD_MS         100     // task_2: FSM tick (ms)
#define REPORT_PERIOD_MS       500     // task_3: LCD + serial report (ms)
#define SERIAL_HEARTBEAT_MS  10000     // task_3: heartbeat force-print (ms)

// --- Hardware pins ----------------------------------------------------------
#define PIN_BTN_NS         2    // NS request button (INPUT_PULLUP, active LOW)
#define PIN_BTN_MODE       3    // Night mode button (INPUT_PULLUP, active LOW)

#define PIN_EV_GREEN       12   // Est-Vest  GREEN  LED
#define PIN_EV_YELLOW      11   // Est-Vest  YELLOW LED
#define PIN_EV_RED         9    // Est-Vest  RED    LED

#define PIN_NS_GREEN       8    // Nord-Sud  GREEN  LED
#define PIN_NS_YELLOW      7    // Nord-Sud  YELLOW LED
#define PIN_NS_RED         6    // Nord-Sud  RED    LED

// --- LCD I2C ----------------------------------------------------------------
#define LCD_I2C_ADDR    0x27

// --- Timing constants (ms) -------------------------------------------------
#define EV_GREEN_MIN_MS   5000   // minimum time EV stays green before NS req
#define EV_GREEN_MS       8000   // EV green phase duration (no request)
#define EV_YELLOW_MS      2000   // EV yellow phase duration
#define ALL_RED_MS        1000   // all-red safety gap
#define NS_GREEN_MS       6000   // NS green phase duration
#define NS_YELLOW_MS      2000   // NS yellow phase duration
#define NIGHT_BLINK_MS     500   // night mode blink half-period
#define NIGHT_PRESS_MS    3000   // kept for compatibility with existing prints

// --- Debounce ---------------------------------------------------------------
#define BTN_DEBOUNCE_MS    50    // ms stable LOW to confirm press

// ---------------------------------------------------------------------------
// FSM states
// ---------------------------------------------------------------------------
typedef enum {
    FSM_EV_GREEN  = 0,
    FSM_EV_YELLOW = 1,
    FSM_ALL_RED_1 = 2,   // before NS_GREEN
    FSM_NS_GREEN  = 3,
    FSM_NS_YELLOW = 4,
    FSM_ALL_RED_2 = 5,   // before EV_GREEN
    FSM_NIGHT     = 6
} TrafficState_t;

// ---------------------------------------------------------------------------
// User command – produced by task_1, consumed by task_2
// ---------------------------------------------------------------------------
typedef struct {
    bool ns_request;      // NS direction has a pending request
    bool toggle_night;    // long press → toggle night mode
    bool force_report;    // STATUS command
    bool changed;
} App72UserCmd_t;

// ---------------------------------------------------------------------------
// Snapshot – produced by task_2, consumed by task_3
// ---------------------------------------------------------------------------
typedef struct {
    TrafficState_t state;        // current FSM state
    bool           ev_green;
    bool           ev_yellow;
    bool           ev_red;
    bool           ns_green;
    bool           ns_yellow;
    bool           ns_red;
    bool           ns_request;   // NS request pending
    bool           night_mode;   // night mode active
    bool           night_blink;  // blink phase (true = yellow ON)
    uint32_t       state_ms;     // ms elapsed in current state
    uint32_t       uptime_ms;
    bool           force_report;
} App72Snapshot_t;

extern App72Snapshot_t   g_app72_snapshot;
extern SemaphoreHandle_t g_app72_snapshot_mutex;
extern SemaphoreHandle_t g_app72_io_mutex;

#endif // APP_LAB_7_2_TASK_CONFIG_H