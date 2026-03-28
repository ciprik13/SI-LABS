#include "task_1.h"
#include "task_config.h"
#include "srv_serial_stdio/srv_serial_stdio.h"
#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <Arduino.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>

// ===========================================================================
// task_1 – Serial command decoder  (20 ms period, priority 3)
//
// Four independent intent registers are assembled into App52UserCmd_t only
// at read time in task1_get_cmd() — no struct is ever partially updated.
//
// AUTO / PWM switching:
//   AUTO            → s_mode = AUTO; limit flags cleared
//   PWM <0..255>    → s_mode = MANUAL; s_pwm_manual = val
//   INC / DEC       → only effective in MANUAL mode; ignored in AUTO
//
// Bonus behaviour:
//   INC/DEC clamp at PWM_MIN / PWM_MAX and set at_limit_* flags.
//   Flags are cleared on the next command that moves away from the boundary.
// ===========================================================================

#define RX_LINE_MAX  40
#define SEM_TICKS    pdMS_TO_TICKS(10)

// ---------------------------------------------------------------------------
// Intent registers  (protected by s_mutex)
// ---------------------------------------------------------------------------
static bool         s_bin_on     = false;
static AnalogMode_t s_mode       = ANALOG_MODE_AUTO;   // default: AUTO
static int          s_pwm_manual = 0;
static bool         s_at_max     = false;
static bool         s_at_min     = false;

static SemaphoreHandle_t s_mutex = NULL;

// ---------------------------------------------------------------------------
// Normalisation passes  (identical structure to Lab 5.1 task_1)
// ---------------------------------------------------------------------------
static void pass_uppercase(char *buf, uint8_t len) {
    for (uint8_t i = 0; i < len; i++)
        buf[i] = (char)toupper((unsigned char)buf[i]);
}

static uint8_t pass_trim(char *buf, uint8_t len) {
    while (len > 0 && buf[len - 1] == ' ') len--;
    buf[len] = '\0';
    uint8_t head = 0;
    while (head < len && buf[head] == ' ') head++;
    if (head > 0) {
        memmove(buf, buf + head, len - head + 1);
        len = (uint8_t)(len - head);
    }
    return len;
}

static bool pass_charset(const char *buf, uint8_t len) {
    for (uint8_t i = 0; i < len; i++) {
        char c = buf[i];
        if (!((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == ' '))
            return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// Keyword handlers
// ---------------------------------------------------------------------------
static void kw_on() {
    if (xSemaphoreTake(s_mutex, SEM_TICKS) == pdTRUE) {
        s_bin_on = true;
        xSemaphoreGive(s_mutex);
    }
    printf("CMD OK: RELAY=ON\n");
}

static void kw_off() {
    if (xSemaphoreTake(s_mutex, SEM_TICKS) == pdTRUE) {
        s_bin_on = false;
        xSemaphoreGive(s_mutex);
    }
    printf("CMD OK: RELAY=OFF\n");
}

static void kw_auto() {
    if (xSemaphoreTake(s_mutex, SEM_TICKS) == pdTRUE) {
        s_mode   = ANALOG_MODE_AUTO;
        s_at_max = false;
        s_at_min = false;
        xSemaphoreGive(s_mutex);
    }
    printf("CMD OK: MOTOR=AUTO (potentiometer)\n");
}

// Bonus: INC – only in MANUAL mode
static void kw_inc() {
    if (xSemaphoreTake(s_mutex, SEM_TICKS) == pdTRUE) {
        if (s_mode == ANALOG_MODE_AUTO) {
            xSemaphoreGive(s_mutex);
            printf("CMD WARN: INC ignored in AUTO mode. Use PWM first.\n");
            return;
        }
        int v    = s_pwm_manual + SPEED_STEP;
        s_at_min = false;
        if (v >= PWM_MAX) { v = PWM_MAX; s_at_max = true; }
        else               { s_at_max = false; }
        s_pwm_manual = v;
        xSemaphoreGive(s_mutex);
        if (s_at_max) printf("CMD OK: PWM=%d [MAX LIMIT REACHED]\n", v);
        else          printf("CMD OK: PWM=%d\n", v);
    }
}

// Bonus: DEC – only in MANUAL mode
static void kw_dec() {
    if (xSemaphoreTake(s_mutex, SEM_TICKS) == pdTRUE) {
        if (s_mode == ANALOG_MODE_AUTO) {
            xSemaphoreGive(s_mutex);
            printf("CMD WARN: DEC ignored in AUTO mode. Use PWM first.\n");
            return;
        }
        int v    = s_pwm_manual - SPEED_STEP;
        s_at_max = false;
        if (v <= PWM_MIN) { v = PWM_MIN; s_at_min = true; }
        else               { s_at_min = false; }
        s_pwm_manual = v;
        xSemaphoreGive(s_mutex);
        if (s_at_min) printf("CMD OK: PWM=%d [MIN LIMIT - MOTOR STOPPED]\n", v);
        else          printf("CMD OK: PWM=%d\n", v);
    }
}

static void kw_help() {
    printf("Commands:\n"
           "  ON | OFF          - relay on/off (+ relay LED)\n"
           "  AUTO              - motor speed tracks potentiometer\n"
           "  PWM <0..255>      - motor speed manual\n"
           "  INC | DEC         - step speed +/-%d (MANUAL mode only)\n"
           "  HELP              - this message\n",
           SPEED_STEP);
}

// ---------------------------------------------------------------------------
// Keyword dispatch table
// ---------------------------------------------------------------------------
typedef void (*keyword_fn_t)(void);
typedef struct { const char *kw; keyword_fn_t fn; } App52CmdEntry_t;

static const App52CmdEntry_t CMD_TABLE[] = {
    { "ON",   kw_on   },
    { "OFF",  kw_off  },
    { "AUTO", kw_auto },
    { "INC",  kw_inc  },
    { "DEC",  kw_dec  },
    { "HELP", kw_help },
};
static const uint8_t CMD_TABLE_LEN =
    (uint8_t)(sizeof(CMD_TABLE) / sizeof(App52CmdEntry_t));

// ---------------------------------------------------------------------------
// decode_line_from_buf
// ---------------------------------------------------------------------------
static void decode_line_from_buf(char *buf) {
    uint8_t len = (uint8_t)strlen(buf);
    if (len == 0) return;

    pass_uppercase(buf, len);
    len = pass_trim(buf, len);
    if (len == 0 || !pass_charset(buf, len)) return;

    // PWM <value> – two-token command
    char kw[8] = {0};
    int  val   = -1;
    if (sscanf(buf, "%7s %d", kw, &val) == 2 && strcmp(kw, "PWM") == 0) {
        if (val < PWM_MIN || val > PWM_MAX) {
            printf("CMD ERR: PWM must be %d..%d\n", PWM_MIN, PWM_MAX);
            return;
        }
        if (xSemaphoreTake(s_mutex, SEM_TICKS) == pdTRUE) {
            s_mode       = ANALOG_MODE_MANUAL;
            s_pwm_manual = val;
            s_at_max     = (val == PWM_MAX);
            s_at_min     = (val == PWM_MIN);
            xSemaphoreGive(s_mutex);
        }
        if (val == PWM_MAX)      printf("CMD OK: PWM=%d [MAX LIMIT]\n", val);
        else if (val == PWM_MIN) printf("CMD OK: PWM=%d [MIN LIMIT - MOTOR STOPPED]\n", val);
        else                     printf("CMD OK: PWM=%d MANUAL\n", val);
        return;
    }

    // No-argument keyword table
    for (uint8_t i = 0; i < CMD_TABLE_LEN; i++) {
        if (strcmp(buf, CMD_TABLE[i].kw) == 0) {
            CMD_TABLE[i].fn();
            return;
        }
    }

    printf("CMD ERR: Unknown command. Use HELP\n");
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
void task1_setup() {
    s_mutex = xSemaphoreCreateMutex();
}

App52UserCmd_t task1_get_cmd() {
    App52UserCmd_t out = { false, ANALOG_MODE_AUTO, 0, false, false };
    if (xSemaphoreTake(s_mutex, SEM_TICKS) == pdTRUE) {
        out.bin_on       = s_bin_on;
        out.analog_mode  = s_mode;
        out.pwm_manual   = s_pwm_manual;
        out.at_limit_max = s_at_max;
        out.at_limit_min = s_at_min;
        xSemaphoreGive(s_mutex);
    }
    return out;
}

// ---------------------------------------------------------------------------
// Task entry point
// ---------------------------------------------------------------------------
void task1_run(void *pvParameters) {
    (void)pvParameters;

    char    rx_line[RX_LINE_MAX] = {0};
    uint8_t rx_pos               = 0;

    printf("Lab 5.2 ready.\n");
    printf("Commands: ON | OFF | AUTO | PWM <0..255> | INC | DEC | HELP\n");

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(CMD_PERIOD_MS));

        while (Serial.available() > 0) {
            char ch = (char)getchar();
            if (ch == '\r' || ch == '\n') {
                if (rx_pos > 0) {
                    rx_line[rx_pos] = '\0';
                    decode_line_from_buf(rx_line);
                    rx_pos = 0;
                }
            } else if (rx_pos < RX_LINE_MAX - 1) {
                rx_line[rx_pos++] = ch;
            }
        }
    }
}
