#include "task_1.h"
#include "task_config.h"
#include "srv_serial_stdio/srv_serial_stdio.h"
#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <Arduino.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

#define RX_LINE_MAX  64
#define SEM_TICKS    pdMS_TO_TICKS(10)

// --- Serial line buffer -----------------------------------------------------
static char rx_line[RX_LINE_MAX] = {0};
static int  rx_pos               = 0;

// --- Shared command state ---------------------------------------------------
static bool s_ns_request    = false;
static bool s_toggle_night  = false;
static bool s_force_report  = false;
static bool s_changed       = false;

static SemaphoreHandle_t s_mutex = NULL;

// --- Button debounce state --------------------------------------------------
typedef struct {
    bool     stable_low;
    uint32_t edge_ms;
} DebounceBtn_t;

static DebounceBtn_t s_btn_ns   = { false, 0 };
static DebounceBtn_t s_btn_mode = { false, 0 };

static inline uint32_t ms_now() {
    return (uint32_t)((uint32_t)xTaskGetTickCount() * portTICK_PERIOD_MS);
}

// ---------------------------------------------------------------------------
// button_tick – called every CMD_PERIOD_MS
// Implements debounce + press event detection for both buttons.
// ---------------------------------------------------------------------------
static void button_tick() {
    uint32_t now = ms_now();

    bool ns_low = (digitalRead(PIN_BTN_NS) == LOW);
    if (ns_low != s_btn_ns.stable_low) {
        if ((now - s_btn_ns.edge_ms) >= BTN_DEBOUNCE_MS) {
            s_btn_ns.stable_low = ns_low;
            s_btn_ns.edge_ms = now;
            if (ns_low && xSemaphoreTake(s_mutex, SEM_TICKS) == pdTRUE) {
                s_ns_request = true;
                s_changed = true;
                xSemaphoreGive(s_mutex);
            }
        }
    } else {
        s_btn_ns.edge_ms = now;
    }

    bool mode_low = (digitalRead(PIN_BTN_MODE) == LOW);
    if (mode_low != s_btn_mode.stable_low) {
        if ((now - s_btn_mode.edge_ms) >= BTN_DEBOUNCE_MS) {
            s_btn_mode.stable_low = mode_low;
            s_btn_mode.edge_ms = now;
            if (mode_low && xSemaphoreTake(s_mutex, SEM_TICKS) == pdTRUE) {
                s_toggle_night = true;
                s_changed = true;
                xSemaphoreGive(s_mutex);
            }
        }
    } else {
        s_btn_mode.edge_ms = now;
    }
}

// ---------------------------------------------------------------------------
// str_uppercase / decode_line
// ---------------------------------------------------------------------------
static void str_uppercase(char *buf) {
    for (int i = 0; buf[i]; i++)
        buf[i] = (char)toupper((unsigned char)buf[i]);
}

static void decode_line(char *buf) {
    char kw[RX_LINE_MAX] = {0};
    if (sscanf(buf, " %63s", kw) < 1) return;
    str_uppercase(kw);

    if (strcmp(kw, "STATUS") == 0) {
        if (xSemaphoreTake(s_mutex, SEM_TICKS) == pdTRUE) {
            s_force_report = true; s_changed = true;
            xSemaphoreGive(s_mutex);
        }
        printf("\rCMD OK: STATUS report queued\n");
        return;
    }
    if (strcmp(kw, "NIGHT") == 0) {
        if (xSemaphoreTake(s_mutex, SEM_TICKS) == pdTRUE) {
            s_toggle_night = true; s_changed = true;
            xSemaphoreGive(s_mutex);
        }
        printf("\rCMD OK: Night mode toggle queued\n");
        return;
    }
    if (strcmp(kw, "REQUEST") == 0) {
        if (xSemaphoreTake(s_mutex, SEM_TICKS) == pdTRUE) {
            s_ns_request = true; s_changed = true;
            xSemaphoreGive(s_mutex);
        }
        printf("\rCMD OK: NS request queued\n");
        return;
    }
    if (strcmp(kw, "HELP") == 0) {
        printf("\r\nLab 7.2 – Smart Traffic Light\r\n"
               "Commands:\r\n"
               "  STATUS   - force serial report\r\n"
               "  REQUEST  - simulate NS direction request\r\n"
               "  NIGHT    - toggle night mode\r\n"
               "  HELP     - this message\r\n");
        return;
    }
    printf("\rCMD ERR: Unknown command '%s'. Use HELP\n", kw);
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
void task1_72_setup() {
    s_mutex = xSemaphoreCreateMutex();
}

App72UserCmd_t task1_72_get_cmd() {
    App72UserCmd_t out = {false, false, false, false};
    if (xSemaphoreTake(s_mutex, SEM_TICKS) == pdTRUE) {
        out.ns_request    = s_ns_request;
        out.toggle_night  = s_toggle_night;
        out.force_report  = s_force_report;
        out.changed       = s_changed;
        // Reset consumed flags atomically
        s_ns_request      = false;
        s_toggle_night    = false;
        s_force_report    = false;
        s_changed         = false;
        xSemaphoreGive(s_mutex);
    }
    return out;
}

// ---------------------------------------------------------------------------
// Task entry point
// ---------------------------------------------------------------------------
void task1_72_run(void *pvParameters) {
    (void)pvParameters;

    printf("\rLab 7.2 Smart Traffic Light ready.\n");
        printf("\rBTN D2 = NS request, BTN D3 = Night mode toggle.\n");
    printf("\rType HELP for commands.\n");

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(CMD_PERIOD_MS));

        button_tick();

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