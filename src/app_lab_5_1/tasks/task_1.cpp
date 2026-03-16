#include "task_1.h"
#include "task_config.h"
#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <Arduino.h>
#include <string.h>
#include <ctype.h>

// ===========================================================================
// Task 1 – Command Input  (50 ms period, priority 3)
//
// Reads one line at a time from Serial (non-blocking; accumulates chars each
// tick).
//
// Accepted commands (case-insensitive):
//   "1" / "ON"   → request binary actuator ON
//   "0" / "OFF"  → request binary actuator OFF
//   "STATUS"     → no state change (display always shows current status)
//
// Any other input sets invalid_cmd = true; the display task shows a warning.
// ===========================================================================

#define CMD_BUF_LEN 16
static char    s_buf[CMD_BUF_LEN];
static uint8_t s_len = 0;

static void to_upper(char *s) {
    while (*s) { *s = (char)toupper((unsigned char)*s); s++; }
}

// Returns 1 (ON), 0 (OFF), or -1 (STATUS / unknown).
// Sets *invalid = true if the command is unrecognised.
static int parse_cmd(char *line, bool *invalid) {
    to_upper(line);
    *invalid = false;

    if (strcmp(line, "1")      == 0 || strcmp(line, "ON")  == 0) return 1;
    if (strcmp(line, "0")      == 0 || strcmp(line, "OFF") == 0) return 0;
    if (strcmp(line, "STATUS") == 0)                              return -1;

    *invalid = true;
    return -1;
}

void task51_cmd_input(void *pvParameters) {
    (void) pvParameters;

    TickType_t xLastWakeTime = xTaskGetTickCount();

    for (;;) {
        int  new_cmd     = -1;
        bool new_invalid = false;

        // ------------------------------------------------------------------
        // Non-blocking Serial read – accumulate chars into s_buf each tick.
        // When '\n' is received the line is parsed as a command.
        // ------------------------------------------------------------------
        while (Serial.available() > 0) {
            char ch = (char)Serial.read();

            if (ch == '\r') continue;  // ignore CR (Windows line endings)

            if (ch == '\n') {
                s_buf[s_len] = '\0';
                if (s_len > 0) {
                    new_cmd = parse_cmd(s_buf, &new_invalid);
                }
                s_len = 0;
                break;  // process one command per tick
            } else {
                if (s_len < CMD_BUF_LEN - 1) {
                    s_buf[s_len++] = ch;
                }
                // Buffer overflow: silently discard extra chars
            }
        }

        // ------------------------------------------------------------------
        // Publish to shared state (only when something actually arrived)
        // ------------------------------------------------------------------
        if (new_cmd != -1 || new_invalid) {
            if (xSemaphoreTake(g51_cmd_mutex, portMAX_DELAY) == pdTRUE) {
                g51_cmd.raw_cmd     = new_cmd;
                g51_cmd.source      = CMD_SRC_SERIAL;
                g51_cmd.invalid_cmd = new_invalid;
                xSemaphoreGive(g51_cmd_mutex);
            }
        }

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(50));
    }
}