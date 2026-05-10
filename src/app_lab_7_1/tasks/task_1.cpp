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
// task_1 – Serial command decoder  (10 ms period, priority 1)
//
// Reading strategy (identical to Lab 6.1 / 6.2):
//   scanf("%c") is called ONLY after Serial.available() confirms a byte is
//   ready, so srv_serial_get_char() returns immediately without blocking.
//   Between 10 ms sleep cycles stdin is idle, so task_3 can printf freely
//   on the shared FILE* stream without collision.
//
// On newline, sscanf parses keyword from the accumulated rx_line buffer.
//
// This lab's FSM has minimal serial commands; the interpreter is intentionally
// lean.  cmd.changed is set on any write to shared state and reset atomically
// inside task1_71_get_cmd() so task_2 sees exactly one notification per event.
// ===========================================================================

#define RX_LINE_MAX  64
#define SEM_TICKS    pdMS_TO_TICKS(10)

static char rx_line[RX_LINE_MAX] = {0};
static int  rx_pos               = 0;

static bool s_force_report = false;
static bool s_changed      = false;

static SemaphoreHandle_t s_mutex = NULL;

static void str_uppercase(char *buf) {
    for (int i = 0; buf[i]; i++)
        buf[i] = (char)toupper((unsigned char)buf[i]);
}

// ---------------------------------------------------------------------------
// decode_line – called once per newline on the complete rx_line buffer.
// sscanf operates on the in-memory buffer, not on stdin.
// ---------------------------------------------------------------------------
static void decode_line(char *buf) {
    char kw[RX_LINE_MAX] = {0};

    int parsed = sscanf(buf, " %63s", kw);
    if (parsed < 1) return;
    str_uppercase(kw);

    // --- STATUS -------------------------------------------------------------
    if (strcmp(kw, "STATUS") == 0) {
        if (xSemaphoreTake(s_mutex, SEM_TICKS) == pdTRUE) {
            s_force_report = true;
            s_changed      = true;
            xSemaphoreGive(s_mutex);
        }
        printf("\rCMD OK: STATUS report queued\n");
        return;
    }

    // --- HELP ---------------------------------------------------------------
    if (strcmp(kw, "HELP") == 0) {
        printf("\r\nLab 7.1 – FSM Button-LED Control\r\n"
               "Commands:\r\n"
               "  STATUS  - force immediate serial report\r\n"
               "  HELP    - this message\r\n");
        return;
    }

    printf("\rCMD ERR: Unknown command '%s'. Use HELP\n", kw);
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
void task1_71_setup() {
    s_mutex = xSemaphoreCreateMutex();
}

App71UserCmd_t task1_71_get_cmd() {
    App71UserCmd_t out;
    out.force_report = false;
    out.changed      = false;

    if (xSemaphoreTake(s_mutex, SEM_TICKS) == pdTRUE) {
        out.force_report = s_force_report;
        out.changed      = s_changed;
        // Reset consumed flags atomically
        s_force_report   = false;
        s_changed        = false;
        xSemaphoreGive(s_mutex);
    }
    return out;
}

// ---------------------------------------------------------------------------
// Task entry point
// ---------------------------------------------------------------------------
void task1_71_run(void *pvParameters) {
    (void)pvParameters;

    printf("\rLab 7.1 FSM Button-LED Control ready.\n");
    printf("\rPress button on pin %d to toggle LED.\n", PIN_BTN);
    printf("\rType HELP for commands.\n");

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(CMD_PERIOD_MS));

        while (Serial.available() > 0) {
            char ch = '\0';
            if (scanf("%c", &ch) != 1) break;

            if (ch == '\r' || ch == '\n') {
                if (rx_pos > 0) {
                    rx_line[rx_pos] = '\0';
                    decode_line(rx_line);
                    rx_pos = 0;
                    memset(rx_line, 0, sizeof(rx_line));
                }
            } else if (rx_pos < RX_LINE_MAX - 1) {
                rx_line[rx_pos++] = ch;
            }
        }
    }
}
