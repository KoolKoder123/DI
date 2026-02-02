#pragma once

// Round 1: Beam-break scoring fills a "jar" with honey.
//
// Refactor notes (no rule changes):
// - The jar only redraws when the score changes.
//   This keeps the LEDs looking the same, but avoids constant NeoPixel.show()
//   calls which can glitch IR remote decoding.
// - Round 1 state is isolated in this file so you can change it later.

#include "leds.h"
#include "beams.h"

// Score tracking: how many *interior* rows are filled in each quadrant.
static uint8_t r1Rows[NUM_STRIPS_CONNECTED] = {0, 0, 0, 0};

// "Dirty" flags: only redraw a quadrant when something changed.
static bool r1Dirty[NUM_STRIPS_CONNECTED] = {true, true, true, true};

// Reset Round 1 scores and force a redraw.
static inline void roundR1Reset() {
  for (int i = 0; i < NUM_STRIPS_CONNECTED; i++) {
    r1Rows[i] = 0;
    r1Dirty[i] = true;
  }
  Serial.println("Round 1: Scores Reset");
}

// Update Round 1 scoring + visuals.
static inline void roundR1Update() {
  // 1) Read beam sensors and update score.
  for (int q = 0; q < NUM_STRIPS_CONNECTED; q++) {
    if (isBeamJustBroken(q)) {
      // Interior can hold up to (QUAD_ROWS - 2) rows.
      int maxInteriorRows = QUAD_ROWS - 2;
      if (r1Rows[q] < maxInteriorRows) {
        r1Rows[q]++;
        r1Dirty[q] = true;
        Serial.print("Point for Quad ");
        Serial.println(q);
      }
    }
  }

  // 2) Redraw only the quadrants that changed.
  for (int q = 0; q < NUM_STRIPS_CONNECTED; q++) {
    if (!r1Dirty[q]) continue;

    // Draw the jar border (fuchsia) + interior honey fill.
    // Keep the same values as the original Round 1 implementation.
    drawJarWithProgress(
      q,
      r1Rows[q],
      strips[q].Color(255, 120, 255),
      240,
      240,
      150
    );

    r1Dirty[q] = false;
  }
}
