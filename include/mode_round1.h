#pragma once
#include "leds.h"
#include "beams.h"

// Round 1: beam sensors fill the "jar" from the bottom up.
static uint8_t r1Rows[4] = {0, 0, 0, 0};

static inline void enterRound1() {
  Serial.println("Round 1: Start");
  ledsAllOff();

  // Reset scores and beam memory.
  for (int i = 0; i < 4; i++) r1Rows[i] = 0;
  beamsReset();
}

static inline void runRound1(bool canShow) {
  if (!canShow) return;

  for (int q = 0; q < NUM_STRIPS_CONNECTED; q++) {
    // 1) Score: a point when the beam is broken.
    if (isBeamJustBroken(q)) {
      int maxInteriorRows = QUAD_ROWS - 2;
      if (r1Rows[q] < maxInteriorRows) {
        r1Rows[q]++;
        Serial.print("Round 1: Point for quadrant ");
        Serial.println(q);
      }
    }

    // 2) Draw: jar border + filled honey interior.
    // Border: bright fuchsia. Interior: honey color.
    drawJarWithProgress(
      q,
      r1Rows[q],
      strips[q].Color(255, 120, 255),  // border color
      240, 240, 150                    // (kept for compatibility)
    );
  }
}
