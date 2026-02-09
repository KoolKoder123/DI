#pragma once
#include "leds.h"
#include "game_state.h"

// Round 3: top quadrants are solid colors + random sparkle flashes.
// Remote buttons (mapped in remote.h) can also shift columns between colors.

// Round 3 relies on project-wide per-column and random-flash state defined
// in `src/main.cpp`. Local r3-prefixed duplicates were unused and removed
// to avoid confusion.

// NOTE: Round 3 entry & per-column-step helpers are handled in main.cpp
// to keep mode_round3.h focused on random-flash helpers used across the
// project. Unused local helpers were removed.

// Update active flashes and restore colors when their duration ends.
static inline void randomFlashUpdate() {
  bool dirty[NUM_STRIPS_CONNECTED] = {false, false, false, false};
  unsigned long now = millis();
  for (int q = Q_TOP_LEFT; q <= Q_TOP_RIGHT; q++) {
    for (uint16_t physIdx = 0; physIdx < LEDS_PER_QUAD; physIdx++) {
      int flat = q * LEDS_PER_QUAD + physIdx;
      if (!randomFlashActive[flat]) continue;
      if (now >= randomFlashEndTime[flat]) {
        // Restore saved color
        strips[q].setPixelColor(physIdx, randomFlashSavedColor[flat]);
        randomFlashActive[flat] = false;
        randomFlashSavedColor[flat] = 0;
        randomFlashEndTime[flat] = 0;
        dirty[q] = true;
      }
    }
  }
  // Push updates for quadrants that changed
  for (int q = Q_TOP_LEFT; q <= Q_TOP_RIGHT; q++) if (dirty[q]) strips[q].show();
}