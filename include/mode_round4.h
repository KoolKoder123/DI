#pragma once
#include "leds.h"
#include "beams.h"

// Round 4: currently the same scoring + jar visuals as Round 1.
// This is separate so you can change Round 4 later without touching Round 1.
static uint8_t r4Rows[4] = {0, 0, 0, 0};

static inline void enterRound4() {
  Serial.println("Round 4: Start");
  ledsAllOff();

  for (int i = 0; i < 4; i++) r4Rows[i] = 0;
  beamsReset();
}

static inline void runRound4(bool canShow) {
  if (!canShow) return;

  for (int q = 0; q < NUM_STRIPS_CONNECTED; q++) {
    if (isBeamJustBroken(q)) {
      int maxInteriorRows = QUAD_ROWS - 2;
      if (r4Rows[q] < maxInteriorRows) {
        r4Rows[q]++;
        Serial.print("Round 4: Point for quadrant ");
        Serial.println(q);
      }
    }

    drawJarWithProgress(
      q,
      r4Rows[q],
      strips[q].Color(255, 120, 255),
      240, 240, 150
    );
  }
}
