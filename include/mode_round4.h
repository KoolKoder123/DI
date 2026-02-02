#pragma once

// Round 4: Starts as a copy of Round 1.
//
// IMPORTANT:
// - This file exists so you can change Round 4 later without touching Round 1.
// - Today, it uses the same scoring + visuals as Round 1 (no logic change).
// - Like Round 1, it only redraws when the score changes to keep IR remote
//   decoding stable.

#include "leds.h"
#include "beams.h"

// Score tracking: how many *interior* rows are filled in each quadrant.
static uint8_t r4Rows[NUM_STRIPS_CONNECTED] = {0, 0, 0, 0};

// "Dirty" flags: only redraw a quadrant when something changed.
static bool r4Dirty[NUM_STRIPS_CONNECTED] = {true, true, true, true};

// Reset Round 4 scores and force a redraw.
static inline void roundR4Reset() {
  for (int i = 0; i < NUM_STRIPS_CONNECTED; i++) {
    r4Rows[i] = 0;
    r4Dirty[i] = true;
  }
  Serial.println("Round 4: Scores Reset");
}

// Update Round 4 scoring + visuals.
static inline void roundR4Update() {
  // 1) Read beam sensors and update score.
  for (int q = 0; q < NUM_STRIPS_CONNECTED; q++) {
    if (isBeamJustBroken(q)) {
      int maxInteriorRows = QUAD_ROWS - 2;
      if (r4Rows[q] < maxInteriorRows) {
        r4Rows[q]++;
        r4Dirty[q] = true;
        Serial.print("Point for Quad ");
        Serial.println(q);
      }
    }
  }

  // 2) Redraw only the quadrants that changed.
  for (int q = 0; q < NUM_STRIPS_CONNECTED; q++) {
    if (!r4Dirty[q]) continue;

    drawJarWithProgress(
      q,
      r4Rows[q],
      strips[q].Color(255, 120, 255),
      240,
      240,
      150
    );

    r4Dirty[q] = false;
  }
}
