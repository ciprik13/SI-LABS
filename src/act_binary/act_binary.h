#ifndef ACT_BINARY_H
#define ACT_BINARY_H

#include <stdbool.h>

// ===========================================================================
// act_binary – Binary Actuator Module (LED / Relay)
//
// Public API – all internal state is hidden in act_binary.cpp.
// Any task that needs to interact with the binary actuator uses ONLY
// these functions; direct access to internal fields is not possible.
// ===========================================================================

// Call once before the FreeRTOS scheduler starts.
void act_binary_init(int pin);

// Request a new state (1 = ON, 0 = OFF).
// Thread-safe. Stores the request; debounce is applied in act_binary_tick().
void act_binary_request(int state);

// Run one debounce + hardware-write cycle.
// Call from the conditioning task at a fixed period (e.g. 50 ms).
void act_binary_tick();

// Returns the committed (debounced) output state: 1 = ON, 0 = OFF, -1 = uninitialised.
// Thread-safe.
int act_binary_get_state();

// Returns the pending (not yet committed) candidate state.
// Thread-safe.
int act_binary_get_pending();

// Returns the current debounce confirmation count.
// Thread-safe.
int act_binary_get_bounce_count();

// Returns configuration constant (for display/reporting use)
int act_binary_get_debounce_samples();

#endif // ACT_BINARY_H