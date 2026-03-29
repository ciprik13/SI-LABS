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
#include <stdarg.h>

// ===========================================================================
// task_1 – Serial command decoder  (20 ms period, priority 3)
//
// REFACTORED for non-blocking input:
//   Read one character per cycle using srv_serial_stdio_try_get_char().
//   Manually parse the command line to mimic scanf behavior without blocking.
//   Parse keyword, then (for PWM) parse numeric argument.
// ===========================================================================

#define RX_LINE_MAX  40
#define RX_BUF_MAX  40
#define SEM_TICKS    pdMS_TO_TICKS(10)

// Input state machine
static char s_rx_buf[RX_BUF_MAX] = {0};
static int  s_rx_idx = 0;


// ---------------------------------------------------------------------------
// Intent registers  (protected by s_mutex)
// ---------------------------------------------------------------------------
static bool         s_bin_on     = false;
static AnalogMode_t s_mode       = ANALOG_MODE_AUTO;
static int          s_pwm_manual = 0;
static bool         s_at_max     = false;
static bool         s_at_min     = false;

static SemaphoreHandle_t s_mutex = NULL;

static void print_locked(const char *fmt, ...) {
    if (g_app52_io_mutex != NULL) {
        if (xSemaphoreTake(g_app52_io_mutex, SEM_TICKS) != pdTRUE) return;
    }

    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);

    if (g_app52_io_mutex != NULL) {
        xSemaphoreGive(g_app52_io_mutex);
    }
}

// ---------------------------------------------------------------------------
// Command handlers
// ---------------------------------------------------------------------------
static void kw_on() {
    if (xSemaphoreTake(s_mutex, SEM_TICKS) == pdTRUE) {
        s_bin_on = true;
        xSemaphoreGive(s_mutex);
    }
    print_locked("\rCMD OK: RELAY=ON\n");
}

static void kw_off() {
    if (xSemaphoreTake(s_mutex, SEM_TICKS) == pdTRUE) {
        s_bin_on = false;
        xSemaphoreGive(s_mutex);
    }
    print_locked("\rCMD OK: RELAY=OFF\n");
}

static void kw_auto() {
    if (xSemaphoreTake(s_mutex, SEM_TICKS) == pdTRUE) {
        s_mode   = ANALOG_MODE_AUTO;
        s_at_max = false;
        s_at_min = false;
        xSemaphoreGive(s_mutex);
    }
    print_locked("\rCMD OK: MOTOR=AUTO (potentiometer)\n");
}

static void kw_inc() {
    if (xSemaphoreTake(s_mutex, SEM_TICKS) == pdTRUE) {
        if (s_mode == ANALOG_MODE_AUTO) {
            xSemaphoreGive(s_mutex);
            print_locked("\rCMD WARN: INC ignored in AUTO mode. Use PWM first.\n");
            return;
        }
        int v    = s_pwm_manual + SPEED_STEP;
        s_at_min = false;
        if (v >= PWM_MAX) { v = PWM_MAX; s_at_max = true; }
        else               { s_at_max = false; }
        s_pwm_manual = v;
        xSemaphoreGive(s_mutex);
        if (s_at_max) print_locked("\rCMD OK: PWM=%d [MAX LIMIT REACHED]\n", v);
        else          print_locked("\rCMD OK: PWM=%d\n", v);
    }
}

static void kw_dec() {
    if (xSemaphoreTake(s_mutex, SEM_TICKS) == pdTRUE) {
        if (s_mode == ANALOG_MODE_AUTO) {
            xSemaphoreGive(s_mutex);
            print_locked("\rCMD WARN: DEC ignored in AUTO mode. Use PWM first.\n");
            return;
        }
        int v    = s_pwm_manual - SPEED_STEP;
        s_at_max = false;
        if (v <= PWM_MIN) { v = PWM_MIN; s_at_min = true; }
        else               { s_at_min = false; }
        s_pwm_manual = v;
        xSemaphoreGive(s_mutex);
        if (s_at_min) print_locked("\rCMD OK: PWM=%d [MIN LIMIT - MOTOR STOPPED]\n", v);
        else          print_locked("\rCMD OK: PWM=%d\n", v);
    }
}

static void kw_help() {
    print_locked("\rCommands:\n"
                 "  ON | OFF          - relay on/off\n"
                 "  AUTO              - motor speed tracks potentiometer\n"
                 "  PWM <0..255>      - motor speed manual\n"
                 "  INC | DEC         - step speed +/-%d (MANUAL mode only)\n"
                 "  HELP              - this message\n",
                 SPEED_STEP);
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
// execute_command_line – Forward declaration
// ---------------------------------------------------------------------------
static void parse_and_execute_line(const char *line, int len);

// ---------------------------------------------------------------------------
// Task entry point – Non-blocking character-by-character input
// Manual parsing that mimics scanf behavior without blocking
// ---------------------------------------------------------------------------
void task1_run(void *pvParameters) {
    (void)pvParameters;

    print_locked("\rLab 5.2 ready.\n");
    print_locked("\rCommands: ON | OFF | AUTO | PWM <0..255> | INC | DEC | HELP\n");

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(CMD_PERIOD_MS));

        // Read ONE character non-blocking per cycle
        char ch = '\0';
        if (srv_serial_stdio_try_get_char(&ch)) {
            // Echo the character
            putchar(ch);
            
            // Handle backspace
            if (ch == '\b' || ch == 0x7F) {
                if (s_rx_idx > 0) {
                    s_rx_idx--;
                    putchar(' ');
                    putchar('\b');
                }
                continue;
            }

            // Handle newline – end of command line
            if (ch == '\n' || ch == '\r') {
                putchar('\n');
                s_rx_buf[s_rx_idx] = '\0';

                // Parse and execute the buffered command line
                if (s_rx_idx > 0) {
                    parse_and_execute_line(s_rx_buf, s_rx_idx);
                }

                s_rx_idx = 0;
                memset(s_rx_buf, 0, sizeof(s_rx_buf));
                continue;
            }

            // Accumulate printable character in buffer
            if (s_rx_idx < RX_BUF_MAX - 1 && isprint((unsigned char)ch)) {
                s_rx_buf[s_rx_idx++] = ch;
            }
        }
    }
}

// ---------------------------------------------------------------------------
// parse_and_execute_line – Manual parsing mimicking scanf behavior
// Parses: KEYWORD [ARG]
// Example: "PWM 210" or "ON" or "HELP"
// ---------------------------------------------------------------------------
static void parse_and_execute_line(const char *line, int len) {
    char keyword[RX_LINE_MAX] = {0};
    int  kw_idx = 0;
    int  i = 0;

    // Skip leading whitespace
    while (i < len && isspace((unsigned char)line[i])) i++;

    // Read keyword (alphabetic characters only)
    while (i < len && isalpha((unsigned char)line[i]) && kw_idx < RX_LINE_MAX - 1) {
        keyword[kw_idx++] = (char)toupper((unsigned char)line[i]);
        i++;
    }
    keyword[kw_idx] = '\0';

    if (kw_idx == 0) return;  // Empty command

    // --- Handle commands ---
    if (strcmp(keyword, "PWM") == 0) {
        // Expected: "PWM <number>"
        // Skip whitespace after keyword
        while (i < len && isspace((unsigned char)line[i])) i++;

        // Parse integer
        int val = 0;
        int digit_count = 0;
        while (i < len && isdigit((unsigned char)line[i]) && digit_count < 3) {
            val = val * 10 + (line[i] - '0');
            i++;
            digit_count++;
        }

        // Validate PWM range
        if (digit_count == 0) {
            print_locked("\rCMD ERR: PWM requires a numeric value\n");
            return;
        }

        if (val < PWM_MIN || val > PWM_MAX) {
            print_locked("\rCMD ERR: PWM must be %d..%d\n", PWM_MIN, PWM_MAX);
            return;
        }

        // Update PWM state
        if (xSemaphoreTake(s_mutex, SEM_TICKS) == pdTRUE) {
            s_mode       = ANALOG_MODE_MANUAL;
            s_pwm_manual = val;
            s_at_max     = (val == PWM_MAX);
            s_at_min     = (val == PWM_MIN);
            xSemaphoreGive(s_mutex);
        }

        if (val == PWM_MAX)      print_locked("\rCMD OK: PWM=%d [MAX LIMIT]\n", val);
        else if (val == PWM_MIN) print_locked("\rCMD OK: PWM=%d [MIN LIMIT - MOTOR STOPPED]\n", val);
        else                     print_locked("\rCMD OK: PWM=%d MANUAL\n", val);

    } else if (strcmp(keyword, "ON") == 0) {
        kw_on();
    } else if (strcmp(keyword, "OFF") == 0) {
        kw_off();
    } else if (strcmp(keyword, "AUTO") == 0) {
        kw_auto();
    } else if (strcmp(keyword, "INC") == 0) {
        kw_inc();
    } else if (strcmp(keyword, "DEC") == 0) {
        kw_dec();
    } else if (strcmp(keyword, "HELP") == 0) {
        kw_help();
    } else {
        print_locked("\rCMD ERR: Unknown command. Use HELP\n");
    }
}
