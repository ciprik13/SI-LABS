#include "task_2.h"
#include "dd_sns_temperature/dd_sns_temperature.h"
#include "dd_sns_dht/dd_sns_dht.h"
#include "dd_led/dd_led.h"
#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <string.h>

// ===========================================================================
// Shared conditioning state – owned here, exposed via task_config.h
// ===========================================================================
CondFull42_t      g42_cond1        = { 0, 0, 0, 0, false, false, 0 };
SemaphoreHandle_t g42_cond1_mutex  = NULL;

CondFull42_t      g42_cond2        = { 0, 0, 0, 0, false, false, 0 };
SemaphoreHandle_t g42_cond2_mutex  = NULL;

void task42_init() {
    g42_cond1_mutex = xSemaphoreCreateMutex();
    g42_cond2_mutex = xSemaphoreCreateMutex();
}

// ===========================================================================
// Circular-buffer filter state (one instance per sensor, private to this task)
// ===========================================================================
typedef struct {
    int buf[FILTER_WINDOW]; // circular buffer of saturated samples
    int head;               // index of next write slot
    int count;              // number of valid samples (0..FILTER_WINDOW)
} FilterBuf_t;

static FilterBuf_t s_f1 = { {0}, 0, 0 };
static FilterBuf_t s_f2 = { {0}, 0, 0 };

// ---------------------------------------------------------------------------
// Push a new sample into the circular buffer.
// ---------------------------------------------------------------------------
static void filter_push(FilterBuf_t *f, int val) {
    f->buf[f->head] = val;
    f->head = (f->head + 1) % FILTER_WINDOW;
    if (f->count < FILTER_WINDOW) f->count++;
}

// ---------------------------------------------------------------------------
// Median (salt-and-pepper) filter.
// Sorts a copy of the buffer (insertion sort – cheap for tiny windows)
// and returns the middle element.
// ---------------------------------------------------------------------------
static int filter_median(const FilterBuf_t *f) {
    if (f->count == 0) return 0;

    int tmp[FILTER_WINDOW];
    // Copy in chronological order (oldest first)
    for (int i = 0; i < f->count; i++) {
        int idx = (f->head - f->count + i + FILTER_WINDOW) % FILTER_WINDOW;
        tmp[i] = f->buf[idx];
    }
    // Insertion sort
    for (int i = 1; i < f->count; i++) {
        int key = tmp[i];
        int j   = i - 1;
        while (j >= 0 && tmp[j] > key) { tmp[j + 1] = tmp[j]; j--; }
        tmp[j + 1] = key;
    }
    return tmp[f->count / 2];
}

// ---------------------------------------------------------------------------
// Weighted moving average.
// Weight ramp: oldest sample gets weight 1, newest gets weight `count`.
// This gives more influence to recent measurements while retaining history.
// ---------------------------------------------------------------------------
static int filter_weighted(const FilterBuf_t *f) {
    if (f->count == 0) return 0;

    long sum   = 0;
    long total = 0;
    for (int i = 0; i < f->count; i++) {
        int w   = i + 1;  // weight 1..count (oldest → newest)
        int idx = (f->head - f->count + i + FILTER_WINDOW) % FILTER_WINDOW;
        sum   += (long)f->buf[idx] * w;
        total += w;
    }
    return (int)(sum / total);
}

// ---------------------------------------------------------------------------
// Saturation clamp.
// ---------------------------------------------------------------------------
static int saturate(int val, int lo, int hi) {
    if (val < lo) return lo;
    if (val > hi) return hi;
    return val;
}

// ---------------------------------------------------------------------------
// Generic hysteresis + debounce update.
// Returns whether the committed alert is now active.
// ---------------------------------------------------------------------------
static bool update_alert(CondFull42_t *s, SemaphoreHandle_t mtx,
                         int value,
                         int thresh_hi, int thresh_lo,
                         int antibounce)
{
    bool cur_alert = false;
    if (xSemaphoreTake(mtx, portMAX_DELAY) == pdTRUE) {
        cur_alert = s->alert_active;
        xSemaphoreGive(mtx);
    }

    // Hysteresis: only switch desired state at the edges
    bool desired;
    if      (!cur_alert && value > thresh_hi) desired = true;
    else if ( cur_alert && value < thresh_lo) desired = false;
    else                                      desired = cur_alert;

    bool committed = false;
    if (xSemaphoreTake(mtx, portMAX_DELAY) == pdTRUE) {
        if (desired == s->pending_state) {
            s->bounce_count++;
        } else {
            s->pending_state = desired;
            s->bounce_count  = 1;
        }
        if (s->bounce_count >= antibounce) {
            s->alert_active = s->pending_state;
            s->bounce_count = antibounce; // cap so it doesn't overflow
        }
        committed = s->alert_active;
        xSemaphoreGive(mtx);
    }
    return committed;
}

// ===========================================================================
// Task 2 – Signal Conditioning  (50 ms, +10 ms startup offset)
//
// Pipeline per sensor:
//   raw (driver) → saturation → push to circular buffer
//               → median filter → weighted moving average
//               → hysteresis + debounce → committed alert state
//
// Combined LED indicator (most critical state wins):
//   RED    = at least one sensor in committed ALERT
//   YELLOW = at least one sensor PENDING (transitioning)
//   GREEN  = both sensors OK
// ===========================================================================
void task42_conditioning(void *pvParameters) {
    (void) pvParameters;

    // 10 ms offset – ensures task42_acquisition has already run at least once
    vTaskDelay(pdMS_TO_TICKS(10));
    TickType_t xLastWakeTime = xTaskGetTickCount();

    for (;;) {
        // ===================================================================
        // S1 – Potentiometer (analog temperature simulator)
        // ===================================================================
        int raw1 = dd_sns_temperature_get_celsius();
        int sat1 = saturate(raw1, SAT_LOW_1, SAT_HIGH_1);

        filter_push(&s_f1, sat1);
        int med1 = filter_median(&s_f1);
        int wgt1 = filter_weighted(&s_f1);

        bool committed1 = update_alert(&g42_cond1, g42_cond1_mutex,
                                       wgt1,
                                       ALERT42_THRESHOLD_HIGH_1,
                                       ALERT42_THRESHOLD_LOW_1,
                                       ANTIBOUNCE42_SAMPLES_1);

        // Store intermediate values (for task_3 report)
        if (xSemaphoreTake(g42_cond1_mutex, portMAX_DELAY) == pdTRUE) {
            g42_cond1.raw       = raw1;
            g42_cond1.saturated = sat1;
            g42_cond1.median    = med1;
            g42_cond1.weighted  = wgt1;
            xSemaphoreGive(g42_cond1_mutex);
        }

        // ===================================================================
        // S2 – DHT22 (digital temperature + humidity)
        // ===================================================================
        int raw2 = dd_sns_dht_get_celsius();
        int sat2 = saturate(raw2, SAT_LOW_2, SAT_HIGH_2);

        filter_push(&s_f2, sat2);
        int med2 = filter_median(&s_f2);
        int wgt2 = filter_weighted(&s_f2);

        bool committed2 = update_alert(&g42_cond2, g42_cond2_mutex,
                                       wgt2,
                                       ALERT42_THRESHOLD_HIGH_2,
                                       ALERT42_THRESHOLD_LOW_2,
                                       ANTIBOUNCE42_SAMPLES_2);

        if (xSemaphoreTake(g42_cond2_mutex, portMAX_DELAY) == pdTRUE) {
            g42_cond2.raw       = raw2;
            g42_cond2.saturated = sat2;
            g42_cond2.median    = med2;
            g42_cond2.weighted  = wgt2;
            xSemaphoreGive(g42_cond2_mutex);
        }

        // ===================================================================
        // Combined LED indicator
        // ===================================================================
        bool pending1 = false, pending2 = false;
        if (xSemaphoreTake(g42_cond1_mutex, portMAX_DELAY) == pdTRUE) {
            pending1 = (g42_cond1.pending_state != g42_cond1.alert_active) &&
                       (g42_cond1.bounce_count > 0);
            xSemaphoreGive(g42_cond1_mutex);
        }
        if (xSemaphoreTake(g42_cond2_mutex, portMAX_DELAY) == pdTRUE) {
            pending2 = (g42_cond2.pending_state != g42_cond2.alert_active) &&
                       (g42_cond2.bounce_count > 0);
            xSemaphoreGive(g42_cond2_mutex);
        }

        if (committed1 || committed2) {
            dd_led_turn_on();    // RED
            dd_led_1_turn_off(); // GREEN
            dd_led_2_turn_off(); // YELLOW
        } else if (pending1 || pending2) {
            dd_led_turn_off();   // RED
            dd_led_1_turn_off(); // GREEN
            dd_led_2_turn_on();  // YELLOW
        } else {
            dd_led_turn_off();   // RED
            dd_led_1_turn_on();  // GREEN
            dd_led_2_turn_off(); // YELLOW
        }
        dd_led_apply();

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(50));
    }
}
