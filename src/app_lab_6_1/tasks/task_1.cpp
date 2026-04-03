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
// Reading strategy (identical to Lab 5.1):
//   Serial.available() is checked BEFORE getchar() so srv_serial_get_char()
//   never busy-waits — it only runs when a byte is already in the UART
//   buffer and returns immediately.  task_1 sleeps vTaskDelay(20ms) between
//   drain cycles, so task_2 and task_3 can printf freely without contention
//   on the shared stdin/stdout FILE* stream.
//
// Why not scanf directly:
//   scanf blocks holding the shared FILE* stream open until the user presses
//   Enter.  During that wait, task_3's printf also tries to write to stdout
//   (same stream) and blocks — stopping plotter output and LCD reports.
//
// Parsing:
//   Characters drain into rx_line.  On newline, sscanf extracts the keyword
//   and optional integer argument in one pass, same as Lab 5.2 task_1.
// ===========================================================================

#define RX_LINE_MAX  48
#define SEM_TICKS    pdMS_TO_TICKS(10)

// --- Rx accumulator ---------------------------------------------------------
static char rx_line[RX_LINE_MAX] = {0};
static int  rx_pos               = 0;

// --- Intent registers (protected by s_mutex) --------------------------------
static int  s_sp           = SP_DEFAULT;
static int  s_hyst         = HYST_DEFAULT;
static bool s_force_report = false;

static SemaphoreHandle_t s_mutex = NULL;

// ---------------------------------------------------------------------------
// Normalisation
// ---------------------------------------------------------------------------
static void str_uppercase(char *buf) {
    for (int i = 0; buf[i]; i++)
        buf[i] = (char)toupper((unsigned char)buf[i]);
}

// ---------------------------------------------------------------------------
// decode_line – sscanf keyword + optional int, then dispatch
// ---------------------------------------------------------------------------
static void decode_line(char *buf) {
    char kw[RX_LINE_MAX] = {0};
    int  val    = 0;
    int  parsed = sscanf(buf, " %47s %d", kw, &val);
    if (parsed < 1) return;

    str_uppercase(kw);
    bool has_val = (parsed >= 2);

    if (strcmp(kw, "SP+") == 0) {
        if (xSemaphoreTake(s_mutex, SEM_TICKS) == pdTRUE) {
            int v = s_sp + SP_STEP;
            if (v > SP_MAX) v = SP_MAX;
            s_sp = v;
            xSemaphoreGive(s_mutex);
            printf("\rCMD OK: SP=%d\xc2\xb0""C\n", v);
        }
        return;
    }

    if (strcmp(kw, "SP-") == 0) {
        if (xSemaphoreTake(s_mutex, SEM_TICKS) == pdTRUE) {
            int v = s_sp - SP_STEP;
            if (v < SP_MIN) v = SP_MIN;
            s_sp = v;
            xSemaphoreGive(s_mutex);
            printf("\rCMD OK: SP=%d\xc2\xb0""C\n", v);
        }
        return;
    }

    if (strcmp(kw, "SP") == 0) {
        if (!has_val) { printf("\rCMD ERR: SP requires a value. Try SP+, SP-, or SP <val>\n"); return; }
        if (val < SP_MIN || val > SP_MAX) { printf("\rCMD ERR: SP must be %d..%d\n", SP_MIN, SP_MAX); return; }
        if (xSemaphoreTake(s_mutex, SEM_TICKS) == pdTRUE) { s_sp = val; xSemaphoreGive(s_mutex); }
        printf("\rCMD OK: SP=%d\xc2\xb0""C\n", val);
        return;
    }

    if (strcmp(kw, "HYST") == 0) {
        if (!has_val) { printf("\rCMD ERR: HYST requires a value\n"); return; }
        if (val < HYST_MIN || val > HYST_MAX) { printf("\rCMD ERR: HYST must be %d..%d\n", HYST_MIN, HYST_MAX); return; }
        if (xSemaphoreTake(s_mutex, SEM_TICKS) == pdTRUE) { s_hyst = val; xSemaphoreGive(s_mutex); }
        printf("\rCMD OK: HYST=%d\xc2\xb0""C\n", val);
        return;
    }

    if (strcmp(kw, "STATUS") == 0) {
        if (xSemaphoreTake(s_mutex, SEM_TICKS) == pdTRUE) { s_force_report = true; xSemaphoreGive(s_mutex); }
        printf("\rCMD OK: STATUS report queued\n");
        return;
    }

    if (strcmp(kw, "HELP") == 0) {
        printf("\r\nCommands:\r\n"
               "  SP <val>     - set setpoint (%d..%d \xc2\xb0""C)\r\n"
               "  SP+          - increment setpoint by %d \xc2\xb0""C\r\n"
               "  SP-          - decrement setpoint by %d \xc2\xb0""C\r\n"
               "  HYST <val>   - set hysteresis band (%d..%d \xc2\xb0""C)\r\n"
               "  STATUS       - force serial status report\r\n"
               "  HELP         - this message\r\n",
               SP_MIN, SP_MAX, SP_STEP, SP_STEP, HYST_MIN, HYST_MAX);
        return;
    }

    printf("\rCMD ERR: Unknown command '%s'. Use HELP\n", kw);
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
void task1_61_setup() {
    s_mutex = xSemaphoreCreateMutex();
}

App61UserCmd_t task1_61_get_cmd() {
    App61UserCmd_t out = { false, SP_DEFAULT, false, HYST_DEFAULT, false };
    if (xSemaphoreTake(s_mutex, SEM_TICKS) == pdTRUE) {
        out.sp           = s_sp;
        out.hyst         = s_hyst;
        out.force_report = s_force_report;
        s_force_report   = false;
        xSemaphoreGive(s_mutex);
    }
    return out;
}

// ---------------------------------------------------------------------------
// Task entry point
//
// Sleeps 20ms between drain cycles.  During that sleep task_3 can printf
// freely.  getchar() is only called after Serial.available() confirms a
// byte is ready, so it returns immediately — stream is never held open.
// ---------------------------------------------------------------------------
void task1_61_run(void *pvParameters) {
    (void)pvParameters;

    printf("\rLab 6.1 ON-OFF Hysteresis Control ready.\n");
    printf("\rSP=%d\xc2\xb0""C  HYST=%d\xc2\xb0""C  Heater relay on pin %d\n",
           SP_DEFAULT, HYST_DEFAULT, PIN_RELAY);
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