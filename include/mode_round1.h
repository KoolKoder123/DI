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

// Per-quadrant dropping 2x2-square animation state
static bool dropActive[NUM_STRIPS_CONNECTED] = {false, false, false, false};
static uint8_t dropX[NUM_STRIPS_CONNECTED] = {0, 0, 0, 0};
// dropTopY: the Y coordinate of the top row of the 2x2 square
static int8_t dropTopY[NUM_STRIPS_CONNECTED] = {0, 0, 0, 0};
// phase: 0 = falling, 1 = bottom dissolve (6-on), 2 = top-off -> full row
static uint8_t dropPhase[NUM_STRIPS_CONNECTED] = {0, 0, 0, 0};
static unsigned long dropNextTick[NUM_STRIPS_CONNECTED] = {0, 0, 0, 0};

// Track the previous "eliminated" state so we can force a redraw when it changes.
static bool r1PrevBottomLeftEliminated = false;

// --- Round 1 drawing helper ---
// Draw a "jar" with a filled interior.
// If overlayRedX is true, we draw a red X over whatever is already drawn.
static inline void drawRound1JarWithProgress(uint8_t q, uint8_t rows, uint32_t borderColor, bool overlayRedX) {
  if (q >= NUM_STRIPS_CONNECTED) return;

  strips[q].clear();

  // Left border: first 2 columns
  // Border color is intentionally static white and must not be changed by overlays.
  uint32_t borderCol = strips[q].Color(255, 255, 255);
  for (uint8_t y = 0; y < QUAD_ROWS; y++) {
    for (uint8_t x = 0; x < 2; x++) {
      strips[q].setPixelColor(xyToIndex(x, y), borderCol);
    }
  }

  // Right border: last 2 columns
  for (uint8_t y = 0; y < QUAD_ROWS; y++) {
    for (uint8_t x = QUAD_COLS - 2; x < QUAD_COLS; x++) {
      strips[q].setPixelColor(xyToIndex(x, y), borderCol);
    }
  }

  // Bottom border: first 2 rows
  for (uint8_t y = 0; y < 2; y++) {
    for (uint8_t x = 0; x < QUAD_COLS; x++) {
      strips[q].setPixelColor(xyToIndex(x, y), borderCol);
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
    dropActive[i] = false;
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
      int maxInteriorRows = QUAD_ROWS - 2;
      // If quadrant already full or a drop is in progress, ignore the beam.
      if (r1Rows[q] >= maxInteriorRows || dropActive[q]) continue;

      // Start a new 2x2 drop at the top two rows with randomized X.
      // X must avoid the first two and last two columns and allow the dissolve
      // 6-wide effect to remain inside the interior. Pick in range [4 .. QUAD_COLS-6].
      int minX = 4;
      int maxX = QUAD_COLS - 6; // inclusive
      uint8_t sx = (uint8_t)random(minX, maxX + 1);

      dropActive[q] = true;
      dropX[q] = sx;
      dropTopY[q] = QUAD_ROWS - 1; // top row index
      dropPhase[q] = 0;
      dropNextTick[q] = millis() + 10; // start moving after 10ms
    }
  }

  // 2) Redraw only the quadrants that changed.
  unsigned long now = millis();
  for (int q = 0; q < NUM_STRIPS_CONNECTED; q++) {
    // If a drop animation is active, handle its timing and drawing here.
    if (dropActive[q]) {
      if (now >= dropNextTick[q]) {
        int maxInteriorRows = QUAD_ROWS - 2;
        int targetBottomY = 2 + r1Rows[q];

        if (dropPhase[q] == 0) {
          // Falling: move down one row per tick
          dropTopY[q] = dropTopY[q] - 1;
          int bottomY = dropTopY[q] - 1;
          if (bottomY <= targetBottomY) {
            // Finalize immediately: perform the dissolve now so the top two
            // LEDs do not flicker on a later tick.
            dropActive[q] = false;
            dropPhase[q] = 0;
            dropNextTick[q] = 0;

            // Increment filled rows (cap to max) and force redraw.
            if (r1Rows[q] < (QUAD_ROWS - 2)) {
              r1Rows[q]++;
              r1Dirty[q] = true;
              Serial.print("Point for Quad ");
              Serial.println(q);
            }

            // Draw the updated jar immediately (this will show the new row filled)
            bool overlayX = (q == Q_BOTTOM_LEFT) && round1BottomLeftEliminated;
            drawRound1JarWithProgress(q, r1Rows[q], strips[q].Color(255, 120, 255), overlayX);

            // Skip the regular overlay path for this quadrant this tick.
            continue;
          } else {
            dropNextTick[q] = now + 10;
          }
        } else if (dropPhase[q] == 1) {
          // Bottom dissolve: bottom 2 off, 6 middle-on
          dropPhase[q] = 2;
          dropNextTick[q] = now + 10;
        } else if (dropPhase[q] == 2) {
          // Finalize: top 2 off, full row on, finish animation
          dropActive[q] = false;
          dropPhase[q] = 0;
          dropNextTick[q] = 0;

          // Increment filled rows (cap to max)
          if (r1Rows[q] < (QUAD_ROWS - 2)) {
            r1Rows[q]++;
            r1Dirty[q] = true;
            Serial.print("Point for Quad ");
            Serial.println(q);
          }
        }
      }

      // Draw base jar (use existing helper) then overlay the drop visuals
      bool overlayX = (q == Q_BOTTOM_LEFT) && round1BottomLeftEliminated;
      drawRound1JarWithProgress(q, r1Rows[q], strips[q].Color(255, 120, 255), overlayX);

      // Overlay according to current phase
      int topY = dropTopY[q];
      int bottomY = topY - 1;
      uint32_t fillColor = strips[q].Color(128, 128, 0);

      if (dropPhase[q] == 0) {
        // Falling: draw 2x2 square at (dropX, topY)
        strips[q].setPixelColor(xyToIndex(dropX[q], topY), fillColor);
        strips[q].setPixelColor(xyToIndex(dropX[q] + 1, topY), fillColor);
        strips[q].setPixelColor(xyToIndex(dropX[q], bottomY), fillColor);
        strips[q].setPixelColor(xyToIndex(dropX[q] + 1, bottomY), fillColor);
      } else if (dropPhase[q] == 1) {
        // Bottom dissolve: bottom two off, 6 in middle on
        // Ensure bottom two are off
        strips[q].setPixelColor(xyToIndex(dropX[q], bottomY), 0);
        strips[q].setPixelColor(xyToIndex(dropX[q] + 1, bottomY), 0);

        int startX = dropX[q] - 2;
        // Clamp startX to remain inside interior columns [2 .. QUAD_COLS-3]
        int minStart = 2;
        int maxStart = QUAD_COLS - 8; // so startX+5 <= QUAD_COLS-3
        if (startX < minStart) startX = minStart;
        if (startX > maxStart) startX = maxStart;
        for (int xi = 0; xi < 6; xi++) {
          strips[q].setPixelColor(xyToIndex(startX + xi, bottomY), fillColor);
        }
        // Keep the top two visible until next phase
        strips[q].setPixelColor(xyToIndex(dropX[q], topY), fillColor);
        strips[q].setPixelColor(xyToIndex(dropX[q] + 1, topY), fillColor);
      } else if (dropPhase[q] == 2) {
        // Finalize: top two off, interior row on (leave borders white)
        strips[q].setPixelColor(xyToIndex(dropX[q], topY), 0);
        strips[q].setPixelColor(xyToIndex(dropX[q] + 1, topY), 0);
        for (int x = 2; x < QUAD_COLS - 2; x++) {
          strips[q].setPixelColor(xyToIndex(x, bottomY), fillColor);
        }
      }

      strips[q].show();
      continue; // skip the regular dirty-only redraw for this quadrant
    }

    if (!r1Dirty[q]) continue;

    // Draw the jar border (fuchsia) + interior honey fill.
    // If bottom-left is eliminated, overlay the red X.
    bool overlayX = (q == Q_BOTTOM_LEFT) && round1BottomLeftEliminated;
    drawRound1JarWithProgress(q, r1Rows[q], strips[q].Color(255, 120, 255), overlayX);

    r1Dirty[q] = false;
  }
}
