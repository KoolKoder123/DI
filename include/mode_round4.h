#pragma once

// Round 4: Beam-break scoring, but with its own visuals.
//
// Changes requested:
// - Each quadrant's jar has a different base color (red/blue/green/white).
// - CODE_ROUND_FINAL can eliminate multiple quadrants by drawing a red X overlay.
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
  // Color choices (border is now always white; fills remain per-quadrant):
  // - Bottom-left fill: BRIGHT YELLOW
  // - Top-left fill: RED
  // - Top-right fill: PINK
  // - Bottom-right fill: BLUE
  uint8_t br = 255, bg = 255, bb = 255; // white border for all jars
  uint8_t fr = 80, fg = 80, fb = 80;     // default fill (dark)

  switch (q) {
    case Q_BOTTOM_LEFT:  fr = 255; fg = 255; fb = 0;   break; // bright yellow
    case Q_TOP_RIGHT:    fr = 80;  fg = 30;  fb = 60;  break; // pink
    case Q_TOP_LEFT:     fr = 80;  fg = 0;   fb = 0;   break; // red
    case Q_BOTTOM_RIGHT: fr = 0;   fg = 0;   fb = 80;  break; // blue
    default:             fr = 80;  fg = 80;  fb = 80;  break;
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

// light up Q_BOTTOM_LEFT quadrant to celebrate a win.
static inline void celebrateWin() {
  uint32_t yellow = strips[0].Color(255, 255, 0);
  
  strips[Q_BOTTOM_LEFT].clear();
  
  // Draw yellow jar borders
  // Left border
  for (uint8_t y = 0; y < QUAD_ROWS; y++) {
    for (uint8_t x = 0; x < 2; x++) {
      strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(x, y), yellow);
    }
  }
  
  // Right border
  for (uint8_t y = 0; y < QUAD_ROWS; y++) {
    for (uint8_t x = QUAD_COLS - 2; x < QUAD_COLS; x++) {
      strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(x, y), yellow);
    }
  }
  
  // Bottom border
  for (uint8_t y = 0; y < 2; y++) {
    for (uint8_t x = 0; x < QUAD_COLS; x++) {
      strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(x, y), yellow);
    }
  }
  
  // TODO: Draw crown
  uint32_t white = strips[0].Color(160, 160, 160);
  for(int a = 3; a < 15; a++) {
    strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(a, 4), white);
  }
  strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(2, 5), white);
  strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(2, 6), white);
  strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(15, 5), white);
  strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(15, 6), white);

  strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(3, 7), white);
  strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(14, 7), white);

  for(int a = 8; a < 15; a++) {
    strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(2, a), white);
    strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(15, a), white);
  }

  strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(3, 15), white);
  strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(14, 15), white);

  strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(4, 14), white);
  strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(13, 14), white);
  strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(4, 13), white);
  strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(13, 13), white);

  strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(5, 12), white);
  strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(12, 12), white);
  strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(5, 11), white);
  strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(12, 11), white);

  strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(6, 10), white);
  strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(11, 10), white);

  strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(7, 11), white);
  strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(7, 12), white);
  strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(7, 13), white);
  strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(8, 14), white);
  strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(7, 16), white);
  strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(10, 11), white);
  strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(10, 12), white);
  strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(10, 13), white);
  strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(9, 14), white);
  strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(10, 16), white);
  strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(7, 15), white);
  strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(10, 15), white);

  strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(8, 17), white);
  strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(9, 17), white);

  // Fill in crown
  uint32_t purple = strips[0].Color(75, 0, 75); // Purple color
  for(int a = 3; a < 15; a++) {
    strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(a, 5), purple);
    strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(a, 6), purple);
    strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(a, 8), purple);
    strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(a, 9), purple);
  }

  for(int a = 4; a < 14; a++) {
    strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(a, 7), purple);
  }

  for(int a = 3; a < 6; a++) {
    strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(a, 10), purple);
  }

  for(int a = 7; a < 11; a++) {
    strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(a, 10), purple);
  }

  for(int a = 12; a < 15; a++) {
    strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(a, 10), purple);
  }

  strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(3, 11), purple);
  strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(4, 11), purple);
  strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(3, 12), purple);
  strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(4, 12), purple);
  strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(3, 13), purple);
  strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(3, 14), purple);
  strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(14, 14), purple);
  strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(14, 13), purple);
  strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(14, 12), purple);
  strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(14, 11), purple);
  
  for(int a = 11; a < 14; a++) {
    strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(8, a), purple);
    strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(9, a), purple);
  }

  strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(13, 11), purple);
  strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(13, 12), purple);

  strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(8, 16), purple);
  strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(9, 16), purple);

  strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(8, 15), purple);
  strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(9, 15), purple);

  strips[Q_BOTTOM_LEFT].show();
}

// Update Round 4 scoring + visuals.
static inline void roundR4Update() {
  // If any eliminated flags changed, force a redraw.
  for (int q = 0; q < NUM_STRIPS_CONNECTED; q++) {
    if (round4Eliminated[q] != r4PrevEliminated[q]) {
      r4Dirty[q] = true;
      r4PrevEliminated[q] = round4Eliminated[q];
      celebrateWin();
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

// Force the Round 4 final visuals immediately and update internal
// state so subsequent `roundR4Update()` calls won't override it.
static inline void round4ForceFinalDisplay() {
  // Ensure internal rows/dirty flags reflect the forced final
  for (int i = 0; i < NUM_STRIPS_CONNECTED; i++) {
    r4Rows[i] = 0;
    // We've drawn the forced final immediately; don't mark dirty so
    // the regular update won't redraw and erase the crown.
    r4Dirty[i] = false;
    // Align previous-eliminated state so update() doesn't try to reapply overlays
    r4PrevEliminated[i] = round4Eliminated[i];
  }

  // Draw final visuals for each quadrant now, then overlay crown.
  for (int q = 0; q < NUM_STRIPS_CONNECTED; q++) {
    drawRound4Progress(q, r4Rows[q], round4Eliminated[q]);
  }

  // If bottom-left is the winner, draw the crown overlay last so it remains visible.
  if (!round4Eliminated[Q_BOTTOM_LEFT]) {
    celebrateWin();
  }
}

