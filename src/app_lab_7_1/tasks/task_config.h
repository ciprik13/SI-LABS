#ifndef APP_LAB_7_1_TASK_CONFIG_H
#define APP_LAB_7_1_TASK_CONFIG_H

#include <Arduino_FreeRTOS.h>
#include <semphr.h>

// ===========================================================================
// task_config.h – Shared types and constants for Lab 7.1
//
// Finite State Machine – Button-LED control
//
// Hardware (identical breadboard to previous labs – do NOT reassign pins):
//   Button   : PIN_BTN        =  2  (INPUT_PULLUP, active LOW)
//   Relay    : PIN_RELAY      =  6  (unused in 7.1, declared for consistency)
//   LED RED  : PIN_LED_RED    =  9  – ON when FSM state = LED_ON
//   LED GREEN: PIN_LED_GREEN  = 12  – ON when FSM state = LED_OFF
//   LED YELL : PIN_LED_YELLOW = 11  – blinks on stuck-button alert
//   DHT11    : PIN_DHT        =  2  (unused in 7.1, declared for consistency)
//   LCD I2C  : 16x2, address LCD_I2C_ADDR = 0x27  (SDA=D20, SCL=D21)
//
// FSM states:
//   LED_OFF  – LED off; waiting for button press
//   LED_ON   – LED on;  waiting for button press or auto-timeout
//
// Transitions:
//   LED_OFF -> LED_ON  : validated button press
//   LED_ON  -> LED_OFF : validated button press  OR  auto-timeout (10 s)
//
// Bonus behaviour (10%):
//   Auto-timeout: FSM returns to LED_OFF automatically after AUTO_TIMEOUT_MS
//   of inactivity in LED_ON state. Countdown shown on LCD row 1.
//
// Debounce:
//   Sample every COND_PERIOD_MS (20 ms).
//   Valid press = DEBOUNCE_SAMPLES consecutive LOW + release edge.
//   Lockout of DEBOUNCE_LOCKOUT_MS after each valid press.
//   Minimum press duration: DEBOUNCE_SAMPLES x COND_PERIOD_MS = 5 x 20 = 100 ms.
//
// Serial commands (case-insensitive):
//   STATUS  - force immediate serial report from task_3
//   HELP    - print command list
//
// LCD 16x2 layout:
//   Row 0:  "FSM  Lab 7.1    "
//   Row 1 (LED_OFF):        "LED: OFF        "
//   Row 1 (LED_ON):         "LED: ON   T-10s "
//   Row 1 (stuck alert):    "LED: ON   !ALT  "
//
// Arduino Serial Plotter (every task_3 tick):
//   State:0  Button:0
// ===========================================================================

// --- Task periods -----------------------------------------------------------
#define CMD_PERIOD_MS           10     // task_1: serial command poll (ms)
#define COND_PERIOD_MS          20     // task_2: button sample / FSM tick (ms)
#define REPORT_PERIOD_MS       500     // task_3: LCD + serial report (ms)
#define SERIAL_HEARTBEAT_MS  10000     // task_3: heartbeat force-print (ms)

// --- Hardware pins (fixed - same breadboard as all previous labs) ----------
#define PIN_BTN            2    // Push-button  (INPUT_PULLUP, active LOW)
#define PIN_RELAY          6    // Relay coil   (unused in 7.1, reserved)
#define PIN_LED_RED        9    // RED   LED    - ON when FSM state = LED_ON
#define PIN_LED_GREEN     12    // GREEN LED    - ON when FSM state = LED_OFF
#define PIN_LED_YELLOW    11    // YELLOW LED   - blinks on stuck-button alert
#define PIN_DHT            2    // DHT11 data   (unused in 7.1, reserved)

// --- LCD I2C ----------------------------------------------------------------
#define LCD_I2C_ADDR    0x27   // I2C address of 16x2 LCD (SDA=D20, SCL=D21)

// --- Debounce parameters ---------------------------------------------------
#define DEBOUNCE_SAMPLES       5    // consecutive LOW samples to confirm press
#define STUCK_THRESHOLD_MS  3000    // ms - button stuck / long-press alarm
#define DEBOUNCE_LOCKOUT_MS  500    // ms - ignore new presses after a valid one

// --- Blink period for YELLOW alert LED -------------------------------------
#define BLINK_PERIOD_MS      250    // ms half-period

// --- Auto-timeout (bonus FSM behaviour) ------------------------------------
// If FSM stays in LED_ON for longer than this with no button press,
// it automatically transitions back to LED_OFF.
#define AUTO_TIMEOUT_MS    10000    // ms - 10 s auto-off timeout

// ---------------------------------------------------------------------------
// FSM states
// ---------------------------------------------------------------------------
typedef enum {
    FSM_LED_OFF = 0,
    FSM_LED_ON  = 1
} FsmState_t;

// ---------------------------------------------------------------------------
// User command - produced by task_1, consumed by task_2
// ---------------------------------------------------------------------------
typedef struct {
    bool force_report;
    bool changed;
} App71UserCmd_t;

// ---------------------------------------------------------------------------
// Snapshot - produced by task_2, consumed by task_3
// ---------------------------------------------------------------------------
typedef struct {
    FsmState_t fsm_state;
    bool       led_on;
    bool       btn_raw;
    bool       btn_debounced;
    bool       btn_stuck;
    uint32_t   press_count;
    bool       force_report;
    uint32_t   timeout_rem_s;   // seconds remaining before auto-timeout (0 = inactive)
    uint32_t   uptime_ms;
} App71Snapshot_t;

extern App71Snapshot_t   g_app71_snapshot;
extern SemaphoreHandle_t g_app71_snapshot_mutex;
extern SemaphoreHandle_t g_app71_io_mutex;

#endif // APP_LAB_7_1_TASK_CONFIG_H