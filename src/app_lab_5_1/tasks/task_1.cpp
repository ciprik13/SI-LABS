#include "task_1.h"
#include "task_config.h"
#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <Arduino.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>

// ===========================================================================
// Task 1 – Command Input  (20 ms period, priority 3)
//
// Strategy: accumulate Serial bytes each tick (non-blocking).
// On '\n': sscanf() on the complete line — guaranteed not to block.
//
// Commands:
//   ON              → binary actuator ON
//   OFF             → binary actuator OFF
//   AUTO            → analog mode: follow potentiometer
//   PWM <0..255>    → analog mode: fixed PWM value
//   HELP            → print command list
// ===========================================================================

#define LINE_BUF_LEN  40
#define MTX_TIMEOUT   pdMS_TO_TICKS(10)

static App5UserCmd_t     s_cmd       = { false, ANALOG_MODE_AUTO, 0 };
static SemaphoreHandle_t s_cmd_mutex = NULL;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static void str_to_upper(char *text) {
    for (int i = 0; text[i] != '\0'; i++)
        text[i] = (char)toupper((unsigned char)text[i]);
}

static char *trim_whitespace(char *text) {
    while (*text != '\0' && isspace((unsigned char)*text)) text++;
    if (*text == '\0') return text;
    char *end = text + strlen(text) - 1;
    while (end > text && isspace((unsigned char)*end)) { *end = '\0'; end--; }
    return text;
}

// Reject lines containing characters outside A-Z, 0-9, space
// (filters serial noise / garbage bytes)
static bool is_charset_valid(const char *text) {
    for (int i = 0; text[i] != '\0'; i++) {
        char ch = text[i];
        if (!((ch >= 'A' && ch <= 'Z') ||
              (ch >= '0' && ch <= '9') ||
               ch == ' ')) return false;
    }
    return true;
}

// Parse "PWM <value>" — returns true and sets *out on success
static bool parse_pwm_value(const char *s, int *out) {
    char *endptr = NULL;
    long v = strtol(s, &endptr, 10);
    if (endptr == s || *endptr != '\0') return false;
    if (v < ANALOG_PWM_MIN || v > ANALOG_PWM_MAX) return false;
    *out = (int)v;
    return true;
}

// ---------------------------------------------------------------------------
// Parse and apply one complete command line
// ---------------------------------------------------------------------------
static void apply_command(const char *raw_line) {
    char buf[LINE_BUF_LEN] = {0};
    strncpy(buf, raw_line, sizeof(buf) - 1);
    char *line = trim_whitespace(buf);
    if (*line == '\0') return;
    str_to_upper(line);
    if (!is_charset_valid(line)) return;   // silently drop noise

    if (strcmp(line, "ON") == 0) {
        if (xSemaphoreTake(s_cmd_mutex, MTX_TIMEOUT) == pdTRUE) {
            s_cmd.bin_requested = true;
            xSemaphoreGive(s_cmd_mutex);
        }
        printf("CMD OK: BIN=ON\n");
        return;
    }

    if (strcmp(line, "OFF") == 0) {
        if (xSemaphoreTake(s_cmd_mutex, MTX_TIMEOUT) == pdTRUE) {
            s_cmd.bin_requested = false;
            xSemaphoreGive(s_cmd_mutex);
        }
        printf("CMD OK: BIN=OFF\n");
        return;
    }

    if (strcmp(line, "AUTO") == 0) {
        if (xSemaphoreTake(s_cmd_mutex, MTX_TIMEOUT) == pdTRUE) {
            s_cmd.analog_mode = ANALOG_MODE_AUTO;
            xSemaphoreGive(s_cmd_mutex);
        }
        printf("CMD OK: ANALOG=AUTO\n");
        return;
    }

    if (strncmp(line, "PWM ", 4) == 0) {
        int pwm = 0;
        if (!parse_pwm_value(line + 4, &pwm)) {
            printf("CMD ERR: PWM must be %d..%d\n", ANALOG_PWM_MIN, ANALOG_PWM_MAX);
            return;
        }
        if (xSemaphoreTake(s_cmd_mutex, MTX_TIMEOUT) == pdTRUE) {
            s_cmd.analog_mode = ANALOG_MODE_MANUAL;
            s_cmd.manual_pwm  = pwm;
            xSemaphoreGive(s_cmd_mutex);
        }
        printf("CMD OK: ANALOG=MANUAL PWM=%d\n", pwm);
        return;
    }

    if (strcmp(line, "HELP") == 0) {
        printf("Commands: ON | OFF | AUTO | PWM <0..255>\n");
        return;
    }

    printf("CMD ERR: Unknown command. Use HELP\n");
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
void task51_task1_init() {
    s_cmd_mutex = xSemaphoreCreateMutex();
}

App5UserCmd_t task51_task1_get_latest() {
    App5UserCmd_t snap = { false, ANALOG_MODE_AUTO, 0 };
    if (xSemaphoreTake(s_cmd_mutex, MTX_TIMEOUT) == pdTRUE) {
        snap = s_cmd;
        xSemaphoreGive(s_cmd_mutex);
    }
    return snap;
}

// ---------------------------------------------------------------------------
// Task body
// ---------------------------------------------------------------------------
void task51_task1(void *pvParameters) {
    (void) pvParameters;

    char    line_buf[LINE_BUF_LEN] = {0};
    int     line_len = 0;

    printf("Lab 5.1 ready. Use: ON | OFF | AUTO | PWM <0..255> | HELP\n");

    TickType_t xLastWakeTime = xTaskGetTickCount();

    for (;;) {
        while (Serial.available() > 0) {
            char ch = (char)Serial.read();

            if (ch == '\r' || ch == '\n') {
                line_buf[line_len] = '\0';
                if (line_len > 0) {
                    apply_command(line_buf);
                }
                line_len = 0;
                line_buf[0] = '\0';
                continue;
            }

            if (line_len < (int)sizeof(line_buf) - 1) {
                line_buf[line_len++] = ch;
            }
        }

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(ACTUATOR_CMD_PERIOD_MS));
    }
}