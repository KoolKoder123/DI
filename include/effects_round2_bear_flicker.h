#pragma once

/*
  effects_round2_bear_flicker.h

  Round 2 uses a "bear face" drawing on each LED quadrant.
  Some quadrants can flicker (bear on / bear off), and CODE_100 can
  run a precise "lose sequence" on the bottom-right quadrant.

  This file contains ONLY the timing / toggling logic.
  The "what to draw" functions live in leds.h.
*/

#include <Arduino.h>
#include "leds.h"
#include "game_state.h"

// --- Lose sequence tuning ---
// Sequence: toggle 10 times at exactly 50ms intervals, then draw X over bear.
static const int LOSE_TOGGLE_COUNT = 10;
static const unsigned long LOSE_TOGGLE_INTERVAL_MS = 50;

// Normal flicker interval (random range)
static const unsigned long FLICKER_SLOW_MIN_MS = 300;
static const unsigned long FLICKER_SLOW_MAX_MS = 600;

// Fast flicker interval (random range)
static const unsigned long FLICKER_FAST_MIN_MS = 20;
static const unsigned long FLICKER_FAST_MAX_MS = 100;

// Fixed very-fast interval used by "lose" flicker flag
static const unsigned long FLICKER_LOSE_FIXED_MS = 40;

// Update the precise lose sequence timing for any quadrants that are active.
static inline void round2UpdateLoseSequences() {
  for (int q = 0; q < NUM_STRIPS_CONNECTED; q++) {
    if (!loseSequenceActive[q]) continue;

    unsigned long now = millis();
    if (now < loseSequenceNextToggle[q]) continue;

    // Toggle visible state
    bearOnPerQuad[q] = !bearOnPerQuad[q];

    if (bearOnPerQuad[q]) {
      if (!(q == Q_BOTTOM_LEFT && bottomLeftLocked)) {
        uint32_t bearColor = strips[0].Color(15, 8, 0);
        drawBearFace(q, strips[0].Color(255, 255, 255), bearColor);
      }
    } else {
      if (!(q == Q_BOTTOM_LEFT && bottomLeftLocked)) {
        strips[q].clear();
        strips[q].show();
      }
    }

    loseSequenceCount[q]++;

    // Schedule next toggle exactly 50ms later
    loseSequenceNextToggle[q] = now + LOSE_TOGGLE_INTERVAL_MS;

    // If we've toggled enough times, finish the sequence
    if (loseSequenceCount[q] >= LOSE_TOGGLE_COUNT) {
      loseSequenceActive[q] = false;
      loseSequenceCount[q] = 0;

      // Ensure final visible state is the bear, then draw X over it
      uint32_t bearColor = strips[0].Color(15, 8, 0);
      drawBearFace(q, strips[0].Color(255, 255, 255), bearColor);

      // Clear any random flash entries for this quadrant so restores won't override
      for (uint16_t physIdx = 0; physIdx < LEDS_PER_QUAD; physIdx++) {
        int flat = q * LEDS_PER_QUAD + physIdx;
        randomFlashActive[flat] = false;
        randomFlashSavedColor[flat] = strips[q].getPixelColor(physIdx);
        randomFlashEndTime[flat] = 0;
      }

      // Draw the X over the bear (overwrite any overlapping pixels)
      drawRedXOver(q);

      // Stop other flicker flags for this quadrant
      flickerActive[q] = false;
      flickerFastPerQuad[q] = false;
      flickerLosePerQuad[q] = false;
      steadyActive[q] = false;
    }
  }
}

// Update normal / fast flicker on any quadrants that are currently flickering.
static inline void round2UpdateFlicker() {
  for (int q = 0; q < NUM_STRIPS_CONNECTED; q++) {
    if (!flickerActive[q]) continue;
    if (steadyActive[q]) continue; // steady quadrants do not flicker
    if (millis() < nextToggleTimePerQuad[q]) continue;

    // Toggle this quadrant's bear state
    bearOnPerQuad[q] = !bearOnPerQuad[q];

    if (bearOnPerQuad[q]) {
      // If the quadrant is locked (bottom-left), skip drawing bear there
      if (!(q == Q_BOTTOM_LEFT && bottomLeftLocked)) {
        uint32_t bearColor = strips[0].Color(15, 8, 0);
        drawBearFace(q, strips[0].Color(255, 255, 255), bearColor);
      }
    } else {
      // Turn off this quadrant unless it's locked to bright red
      if (!(q == Q_BOTTOM_LEFT && bottomLeftLocked)) {
        strips[q].clear();
        strips[q].show();
      }
    }

    // Choose next toggle interval based on quadrant settings
    if (flickerLosePerQuad[q]) {
      // Fixed, deterministic very-fast flicker for CODE_100
      nextToggleTimePerQuad[q] = millis() + FLICKER_LOSE_FIXED_MS;
    } else if (flickerFastPerQuad[q]) {
      nextToggleTimePerQuad[q] = millis() + random(FLICKER_FAST_MIN_MS, FLICKER_FAST_MAX_MS);
    } else {
      nextToggleTimePerQuad[q] = millis() + random(FLICKER_SLOW_MIN_MS, FLICKER_SLOW_MAX_MS);
    }
  }
}

// Convenience: run all Round 2 per-loop LED timing logic.
static inline void round2UpdateBearEffects() {
  // Keep the original order: lose sequence first, then regular flicker.
  round2UpdateLoseSequences();
  round2UpdateFlicker();
}
