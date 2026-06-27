#ifndef _CAN_INJECT_H_
#define _CAN_INJECT_H_

#include "../../devboard/utils/types.h"

// Generic CAN frame injector.
//
// Lets us cyclically transmit arbitrary frames onto the battery CAN bus, driven from
// MQTT (topic <name>/command/CAN_INJECT) or the homescreen "Inject 0x054" button.
// Purpose: experiment with charge-enable / vehicle-state frames (e.g. 0x054) to try to
// drive the retained Tesla master into CHARGE state for native top-of-charge balancing.
// See balancing/FINDINGS.md in the solarassistant project for the full rationale.
//
// Safety: nothing here moves power — these are signalling frames only. Injection is OFF
// after every boot (state is not persisted), so it can never get stuck on across a reset.

#define CAN_INJECT_MAX_JOBS 8

// Add or replace a cyclic injection for frame `id`.
//   interval_ms : transmit period (clamped to >= 10 ms)
//   count       : number of sends, or -1 for "forever until cleared"
// Returns false if the job table is full.
bool can_inject_set(uint32_t id, const uint8_t* data, uint8_t dlc, bool ext, uint32_t interval_ms,
                    int32_t count);

void can_inject_clear(uint32_t id);
void can_inject_clear_all();
uint8_t can_inject_active_count();

// Call once per main-loop iteration; transmits any jobs whose interval has elapsed.
void can_inject_run(unsigned long now_ms);

#endif
