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

// Track the previous "eliminated" state so we can force a redraw when it changes.
static bool r1PrevBottomLeftEliminated = false;

// --- Round 1 drawing helper ---
// Draw a "jar" with a filled interior.
// If overlayRedX is true, we draw a red X over whatever is already drawn.
static inline void drawRound1JarWithProgress(uint8_t q, uint8_t rows, uint32_t borderColor, bool overlayRedX) {
  if (q >= NUM_STRIPS_CONNECTED) return;

  strips[q].clear();

  // Left border: first 2 columns
  for (uint8_t y = 0; y < QUAD_ROWS; y++) {
    for (uint8_t x = 0; x < 2; x++) {
      strips[q].setPixelColor(xyToIndex(x, y), borderColor);
    }
  }

  // Right border: last 2 columns
  for (uint8_t y = 0; y < QUAD_ROWS; y++) {
    for (uint8_t x = QUAD_COLS - 2; x < QUAD_COLS; x++) {
      strips[q].setPixelColor(xyToIndex(x, y), borderColor);
    }
  }

  // Bottom border: first 2 rows
  for (uint8_t y = 0; y < 2; y++) {
    for (uint8_t x = 0; x < QUAD_COLS; x++) {
      strips[q].setPixelColor(xyToIndex(x, y), borderColor);
    }
  }

  // Interior fill: rows 2 and up, inside the borders
  int maxInteriorRows = QUAD_ROWS - 2;
  if (rows > maxInteriorRows) rows = maxInteriorRows;

  // Honey fill color (kept exactly as before)
  uint32_t fillColor = strips[q].Color(128, 128, 0);

  for (uint8_t y = 2; y < 2 + rows; y++) {
    for (uint8_t x = 2; x < QUAD_COLS - 2; x++) {
      strips[q].setPixelColor(xyToIndex(x, y), fillColor);
    }
  }

  // Optional: overlay a red X (do NOT clear first)
  if (overlayRedX) {
    for (uint8_t y = 0; y < QUAD_ROWS; y++) {
      for (uint8_t x = 0; x < QUAD_COLS; x++) {
        int diag1 = (int)x - (int)y;
        int diag2 = (int)x + (int)y - (QUAD_ROWS - 1);
        if (abs(diag1) <= 1 || abs(diag2) <= 1) {
          strips[q].setPixelColor(xyToIndex(x, y), strips[q].Color(255, 0, 0));
        }
      }
    }
  }

  strips[q].show();
}

// Reset Round 1 scores and force a redraw.
static inline void roundR1Reset() {
  for (int i = 0; i < NUM_STRIPS_CONNECTED; i++) {
    r1Rows[i] = 0;
    r1Dirty[i] = true;
  }
  // Clear the "lose" overlay for Round 1.
  round1BottomLeftEliminated = false;
  r1PrevBottomLeftEliminated = false;
  Serial.println("Round 1: Scores Reset");
}

// Update Round 1 scoring + visuals.
static inline void roundR1Update() {
  // If the lose state changed, force a redraw of the bottom-left quadrant.
  if (round1BottomLeftEliminated != r1PrevBottomLeftEliminated) {
    r1Dirty[Q_BOTTOM_LEFT] = true;
    r1PrevBottomLeftEliminated = round1BottomLeftEliminated;
  }

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
    // If bottom-left is eliminated, overlay the red X.
    bool overlayX = (q == Q_BOTTOM_LEFT) && round1BottomLeftEliminated;
    drawRound1JarWithProgress(q, r1Rows[q], strips[q].Color(255, 120, 255), overlayX);

    r1Dirty[q] = false;
  }
}
