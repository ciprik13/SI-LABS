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
// input_handler – Serial command decoder  (20 ms period, priority 3)
//
// Design choices that differ from a naive approach:
//
//  1. State stored as three independent registers (bin_on, use_auto,
//     fixed_level) rather than a single command struct — the struct is
//     assembled only at read time in input_handler_get_cmd().
//
//  2. Line parsing uses a small lookup table of keyword → handler pairs
//     instead of a cascade of strcmp/strncmp branches.  The PWM command is
//     the only one that needs special treatment (numeric argument).
//
//  3. The line buffer is a small struct with its own length field so the
//     accumulator and the flusher share no implicit state beyond the struct.
//
//  4. Normalisation (uppercase + trim + charset filter) is split into three
//     independent passes on the raw buffer rather than done in one function.
// ===========================================================================

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------
#define RX_LINE_MAX   40
#define SEM_TICKS     pdMS_TO_TICKS(10)

// ---------------------------------------------------------------------------
// Independent intent registers  (protected by s_reg_mutex)
// ---------------------------------------------------------------------------
static bool              s_bin_on      = false;
static bool              s_use_auto    = true;
static int               s_fixed_level = 0;
static SemaphoreHandle_t s_reg_mutex   = NULL;

// ---------------------------------------------------------------------------
// Normalisation passes  (each operates on [0..len) in place)
// ---------------------------------------------------------------------------
static void pass_uppercase(char *buf, uint8_t len) {
    for (uint8_t i = 0; i < len; i++)
        buf[i] = (char)toupper((unsigned char)buf[i]);
}

// Returns new length after stripping leading/trailing spaces
static uint8_t pass_trim(char *buf, uint8_t len) {
    // trim tail
    while (len > 0 && buf[len - 1] == ' ') len--;
    buf[len] = '\0';
    // trim head by shifting
    uint8_t head = 0;
    while (head < len && buf[head] == ' ') head++;
    if (head > 0) {
        memmove(buf, buf + head, len - head + 1);
        len = (uint8_t)(len - head);
    }
    return len;
}

// Returns false if any char is outside [A-Z 0-9]
static bool pass_charset(const char *buf, uint8_t len) {
    for (uint8_t i = 0; i < len; i++) {
        char c = buf[i];
        bool ok = (c >= 'A' && c <= 'Z') ||
                  (c >= '0' && c <= '9') ||
                  (c == ' ');
        if (!ok) return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// Keyword handler table
// Each entry: keyword string + handler function pointer.
// The PWM entry is handled separately (needs numeric argument).
// ---------------------------------------------------------------------------
typedef void (*keyword_fn_t)(void);

static void kw_on()   { if (xSemaphoreTake(s_reg_mutex, SEM_TICKS) == pdTRUE) { s_bin_on = true;     xSemaphoreGive(s_reg_mutex); } printf("CMD OK: BIN=ON\n");       }
static void kw_off()  { if (xSemaphoreTake(s_reg_mutex, SEM_TICKS) == pdTRUE) { s_bin_on = false;    xSemaphoreGive(s_reg_mutex); } printf("CMD OK: BIN=OFF\n");      }
static void kw_auto() { if (xSemaphoreTake(s_reg_mutex, SEM_TICKS) == pdTRUE) { s_use_auto = true;   xSemaphoreGive(s_reg_mutex); } printf("CMD OK: ANALOG=AUTO\n"); }
static void kw_help() { printf("Commands: ON | OFF | AUTO | PWM <0..255>\n"); }

typedef struct {
    const char    *keyword;
    keyword_fn_t   handler;
} CmdEntry_t;

static const CmdEntry_t CMD_TABLE[] = {
    { "ON",   kw_on   },
    { "OFF",  kw_off  },
    { "AUTO", kw_auto },
    { "HELP", kw_help },
};
static const uint8_t CMD_TABLE_LEN =
    (uint8_t)(sizeof(CMD_TABLE) / sizeof(CMD_TABLE[0]));

// ---------------------------------------------------------------------------
// decode_line_from_buf – normalise a raw char buffer then route through table
// ---------------------------------------------------------------------------
static void decode_line_from_buf(char *buf) {
    uint8_t len = (uint8_t)strlen(buf);
    if (len == 0) return;

    pass_uppercase(buf, len);
    len = pass_trim(buf, len);
    if (len == 0 || !pass_charset(buf, len)) return;

    // PWM command — use sscanf to extract the numeric argument
    char pwm_keyword[8] = {0};
    int  pwm_arg        = -1;
    if (sscanf(buf, "%7s %d", pwm_keyword, &pwm_arg) == 2 &&
        strcmp(pwm_keyword, "PWM") == 0) {
        int level = pwm_arg;
        if (level < ANALOG_PWM_MIN || level > ANALOG_PWM_MAX) {
            printf("CMD ERR: PWM must be %d..%d\n", ANALOG_PWM_MIN, ANALOG_PWM_MAX);
            return;
        }
        if (xSemaphoreTake(s_reg_mutex, SEM_TICKS) == pdTRUE) {
            s_use_auto    = false;
            s_fixed_level = level;
            xSemaphoreGive(s_reg_mutex);
        }
        printf("CMD OK: ANALOG=MANUAL PWM=%d\n", level);
        return;
    }

    // Table lookup for all other keywords
    for (uint8_t i = 0; i < CMD_TABLE_LEN; i++) {
        if (strcmp(buf, CMD_TABLE[i].keyword) == 0) {
            CMD_TABLE[i].handler();
            return;
        }
    }

    printf("CMD ERR: Unknown command. Use HELP\n");
}



// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
void input_handler_setup() {
    s_reg_mutex = xSemaphoreCreateMutex();
}

// Assemble the three independent registers into the shared struct type
App5UserCmd_t input_handler_get_cmd() {
    App5UserCmd_t out = { false, ANALOG_MODE_AUTO, 0 };
    if (xSemaphoreTake(s_reg_mutex, SEM_TICKS) == pdTRUE) {
        out.bin_requested = s_bin_on;
        out.analog_mode   = s_use_auto ? ANALOG_MODE_AUTO : ANALOG_MODE_MANUAL;
        out.manual_pwm    = s_fixed_level;
        xSemaphoreGive(s_reg_mutex);
    }
    return out;
}

// ---------------------------------------------------------------------------
// Task entry point
//
// Reading strategy:
//   getchar() is safe here because it is called ONLY after Serial.available()
//   confirms a byte is ready — srv_serial_get_char() will not busy-wait.
//   Characters are accumulated into rx_line one getchar() call per byte.
//   On newline the complete line is handed to sscanf for token extraction
//   before decode_line_from_buf processes it.
// ---------------------------------------------------------------------------
void input_handler_run(void *pvParameters) {
    (void)pvParameters;

    char    rx_line[RX_LINE_MAX] = {0};
    uint8_t rx_pos               = 0;

    printf("Lab 5.1 ready. Use: ON | OFF | AUTO | PWM <0..255> | HELP\n");

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(ACTUATOR_CMD_PERIOD_MS));

        // Accumulate bytes with getchar() — safe because Serial.available()
        // is checked first so the underlying get_char never busy-waits.
        while (Serial.available() > 0) {
            char ch = (char)getchar();

            if (ch == '\r' || ch == '\n') {
                if (rx_pos > 0) {
                    rx_line[rx_pos] = '\0';

                    // Use sscanf to extract the first token for dispatch.
                    // For PWM the numeric argument is parsed separately.
                    char token[RX_LINE_MAX] = {0};
                    sscanf(rx_line, " %39s", token);

                    decode_line_from_buf(rx_line);
                    rx_pos = 0;
                }
            } else if (rx_pos < RX_LINE_MAX - 1) {
                rx_line[rx_pos++] = ch;
            }
        }
    }
}