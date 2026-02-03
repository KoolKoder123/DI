#pragma once

// Round 4: Beam-break scoring, but with its own visuals.
//
// Changes requested:
// - Each quadrant's jar has a different base color (red/blue/green/white).
// - CODE_EQ can eliminate multiple quadrants by drawing a red X overlay.
// - Round 4 uses its own progress drawing function so you can change this
//   visualization later without touching Round 1.

#include "leds.h"
#include "beams.h"

// Score tracking: how many *interior* rows are filled in each quadrant.
static uint8_t r4Rows[NUM_STRIPS_CONNECTED] = {0, 0, 0, 0};

// "Dirty" flags: only redraw a quadrant when something changed.
static bool r4Dirty[NUM_STRIPS_CONNECTED] = {true, true, true, true};

// Track previous eliminated state so we can force a redraw when it changes.
static bool r4PrevEliminated[NUM_STRIPS_CONNECTED] = {false, false, false, false};

// --- Round 4 drawing helper ---
// For now we keep the same "jar" shape as Round 1, but each quadrant uses a
// different color and we keep this function local to Round 4.
static inline void getRound4JarColors(uint8_t q, uint32_t* borderColor, uint32_t* fillColor) {
  // Color choices (picked for high contrast with a red X):
  // - Bottom-left (winner quadrant in this round's ending): RED
  // - Top-left: BLUE
  // - Top-right: GREEN
  // - Bottom-right: WHITE
  uint8_t br = 0, bg = 0, bb = 0;
  uint8_t fr = 0, fg = 0, fb = 0;

  switch (q) {
    case Q_BOTTOM_LEFT:  br = 255; bg = 0;   bb = 0;   fr = 80;  fg = 0;  fb = 0;  break; // red
    case Q_TOP_LEFT:     br = 0;   bg = 0;   bb = 255; fr = 0;   fg = 0;  fb = 80; break; // blue
    case Q_TOP_RIGHT:    br = 0;   bg = 255; bb = 0;   fr = 0;   fg = 80; fb = 0;  break; // green
    case Q_BOTTOM_RIGHT: br = 255; bg = 255; bb = 255; fr = 80;  fg = 80; fb = 80; break; // white
    default:             br = 255; bg = 255; bb = 255; fr = 80;  fg = 80; fb = 80; break;
  }

  if (borderColor) *borderColor = strips[0].Color(br, bg, bb);
  if (fillColor) *fillColor = strips[0].Color(fr, fg, fb);
}

static inline void drawRound4Progress(uint8_t q, uint8_t rows, bool overlayRedX) {
  if (q >= NUM_STRIPS_CONNECTED) return;

  uint32_t borderColor = 0;
  uint32_t fillColor = 0;
  getRound4JarColors(q, &borderColor, &fillColor);

  strips[q].clear();

  // Left border
  for (uint8_t y = 0; y < QUAD_ROWS; y++) {
    for (uint8_t x = 0; x < 2; x++) {
      strips[q].setPixelColor(xyToIndex(x, y), borderColor);
    }
  }

  // Right border
  for (uint8_t y = 0; y < QUAD_ROWS; y++) {
    for (uint8_t x = QUAD_COLS - 2; x < QUAD_COLS; x++) {
      strips[q].setPixelColor(xyToIndex(x, y), borderColor);
    }
  }

  // Bottom border
  for (uint8_t y = 0; y < 2; y++) {
    for (uint8_t x = 0; x < QUAD_COLS; x++) {
      strips[q].setPixelColor(xyToIndex(x, y), borderColor);
    }
  }

  // Interior fill
  int maxInteriorRows = QUAD_ROWS - 2;
  if (rows > maxInteriorRows) rows = maxInteriorRows;
  for (uint8_t y = 2; y < 2 + rows; y++) {
    for (uint8_t x = 2; x < QUAD_COLS - 2; x++) {
      strips[q].setPixelColor(xyToIndex(x, y), fillColor);
    }
  }

  // Optional: overlay a red X to show elimination
  if (overlayRedX) {
    for (uint8_t y = 0; y < QUAD_ROWS; y++) {
      for (uint8_t x = 0; x < QUAD_COLS; x++) {
        int diag1 = (int)x - (int)y;
        int diag2 = (int)x + (int)y - (QUAD_ROWS - 1);
        if (abs(diag1) <= 1 || abs(diag2) <= 1) {
          strips[q].setPixelColor(xyToIndex(x, y), strips[0].Color(255, 0, 0));
        }
      }
    }
  }

  strips[q].show();
}

// Reset Round 4 scores and force a redraw.
static inline void roundR4Reset() {
  for (int i = 0; i < NUM_STRIPS_CONNECTED; i++) {
    r4Rows[i] = 0;
    r4Dirty[i] = true;
  }
  // Clear elimination overlays
  for (int i = 0; i < NUM_STRIPS_CONNECTED; i++) {
    round4Eliminated[i] = false;
    r4PrevEliminated[i] = false;
  }
  Serial.println("Round 4: Scores Reset");
}

// Update Round 4 scoring + visuals.
static inline void roundR4Update() {
  // If any eliminated flags changed, force a redraw.
  for (int q = 0; q < NUM_STRIPS_CONNECTED; q++) {
    if (round4Eliminated[q] != r4PrevEliminated[q]) {
      r4Dirty[q] = true;
      r4PrevEliminated[q] = round4Eliminated[q];
    }
  }

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

    // Round 4 uses its own progress drawing.
    drawRound4Progress(q, r4Rows[q], round4Eliminated[q]);

    r4Dirty[q] = false;
  }
}
