#include "can_inject.h"
#include "../../devboard/utils/logging.h"
#include "comm_can.h"

struct InjectJob {
  bool active;
  CAN_frame frame;
  uint32_t interval_ms;
  unsigned long last_ms;
  int32_t remaining;  // -1 = forever
};

static InjectJob jobs[CAN_INJECT_MAX_JOBS];

bool can_inject_set(uint32_t id, const uint8_t* data, uint8_t dlc, bool ext, uint32_t interval_ms,
                    int32_t count) {
  if (dlc > 8) {
    dlc = 8;
  }
  if (interval_ms < 10) {
    interval_ms = 10;  // clamp to a sane minimum to avoid flooding the bus
  }

  // Reuse the slot already holding this id, otherwise take a free one.
  int slot = -1;
  for (int i = 0; i < CAN_INJECT_MAX_JOBS; i++) {
    if (jobs[i].active && jobs[i].frame.ID == id) {
      slot = i;
      break;
    }
  }
  if (slot < 0) {
    for (int i = 0; i < CAN_INJECT_MAX_JOBS; i++) {
      if (!jobs[i].active) {
        slot = i;
        break;
      }
    }
  }
  if (slot < 0) {
    logging.println("CAN inject: job table full");
    return false;
  }

  InjectJob& j = jobs[slot];
  j.frame.FD = false;
  j.frame.ext_ID = ext;
  j.frame.DLC = dlc;
  j.frame.ID = id;
  for (uint8_t k = 0; k < 8; k++) {
    j.frame.data.u8[k] = (k < dlc) ? data[k] : 0;
  }
  j.interval_ms = interval_ms;
  j.last_ms = 0;
  j.remaining = count;
  j.active = true;
  logging.printf("CAN inject: 0x%03X every %lums (count %ld)\n", (unsigned int)id,
                 (unsigned long)interval_ms, (long)count);
  return true;
}

void can_inject_clear(uint32_t id) {
  for (int i = 0; i < CAN_INJECT_MAX_JOBS; i++) {
    if (jobs[i].active && jobs[i].frame.ID == id) {
      jobs[i].active = false;
      logging.printf("CAN inject: cleared 0x%03X\n", (unsigned int)id);
    }
  }
}

void can_inject_clear_all() {
  for (int i = 0; i < CAN_INJECT_MAX_JOBS; i++) {
    jobs[i].active = false;
  }
  logging.println("CAN inject: cleared all");
}

uint8_t can_inject_active_count() {
  uint8_t n = 0;
  for (int i = 0; i < CAN_INJECT_MAX_JOBS; i++) {
    if (jobs[i].active) {
      n++;
    }
  }
  return n;
}

void can_inject_run(unsigned long now_ms) {
  for (int i = 0; i < CAN_INJECT_MAX_JOBS; i++) {
    InjectJob& j = jobs[i];
    if (!j.active) {
      continue;
    }
    if (j.last_ms != 0 && (now_ms - j.last_ms) < j.interval_ms) {
      continue;
    }
    j.last_ms = now_ms ? now_ms : 1;  // avoid 0 so the "first send" check works
    transmit_can_frame_to_interface(&j.frame, can_config.battery);
    if (j.remaining > 0) {
      if (--j.remaining == 0) {
        j.active = false;
      }
    }
  }
}
