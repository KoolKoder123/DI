#pragma once

/*
  effects_random_flashes.h

  Round 3 "sparkle" effect:
  - Every so often, pick random LEDs in the TOP quadrants and flash them
    a random color for a short time.
  - Then restore the original color.

  Why we store memory:
  - Before we change a pixel, we save its old color so we can put it back.
*/

#include <Arduino.h>
#include "leds.h"
#include "game_state.h"

// How often we attempt new flashes
static const unsigned long RANDOM_FLASH_TICK_MS = 100;
// How long a flash stays on before we restore the original color
static const unsigned long RANDOM_FLASH_DURATION_MS = 300;
// How many random "candidates" we try each tick
static const int RANDOM_FLASH_ATTEMPTS_PER_TICK = 30;

// Internal timing variable (only used by the random flash effect)
static unsigned long nextRandomFlashTick = 0;

// Pick random candidate LEDs in top quadrants and possibly start flashes.
static inline void randomFlashTryStart() {
  if (millis() < nextRandomFlashTick) return;
  nextRandomFlashTick = millis() + RANDOM_FLASH_TICK_MS;

  for (int a = 0; a < RANDOM_FLASH_ATTEMPTS_PER_TICK; a++) {
    // Choose top-left or top-right
    int q = (random(0, 2) == 0) ? Q_TOP_LEFT : Q_TOP_RIGHT;

    // Choose a usable coordinate inside QUAD_COLS x QUAD_ROWS
    int x = random(0, QUAD_COLS);
    int y = random(0, QUAD_ROWS);
    uint16_t physIdx = xyToIndex(x, y);
    int flat = q * LEDS_PER_QUAD + physIdx;

    if (randomFlashActive[flat]) continue; // Already flashing

    // 1/10-ish chance to start a flash for this candidate
    if (random(8) != 0) continue;

    // Save current color and start flash with a random color
    uint32_t cur = strips[q].getPixelColor(physIdx);
    randomFlashSavedColor[flat] = cur;

    uint8_t r = random(0, 256);
    uint8_t g = random(0, 256);
    uint8_t b = random(0, 256);
    uint32_t newc = strips[q].Color(r, g, b);
    strips[q].setPixelColor(physIdx, newc);

    randomFlashActive[flat] = true;
    randomFlashEndTime[flat] = millis() + RANDOM_FLASH_DURATION_MS;
  }
}

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
  for (int q = Q_TOP_LEFT; q <= Q_TOP_RIGHT; q++) {
    if (dirty[q]) strips[q].show();
  }
}
