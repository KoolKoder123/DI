#pragma once
#include "leds.h"
#include "beams.h"

// Score tracking: How many rows are filled in each quadrant?
uint8_t r1Rows[4] = {0,0,0,0};

void round1Update() {
  for (int q = 0; q < NUM_STRIPS_CONNECTED; q++) {
    
    // 1. Check if a ball passed through the beam
    if (beamBroken(q)) {
      // Interior can hold up to (QUAD_ROWS - 2) rows
      int maxInteriorRows = QUAD_ROWS - 2;
      if (r1Rows[q] < maxInteriorRows) {
        r1Rows[q]++; // Increase score
        Serial.print("Point for Quad "); Serial.println(q);
      }
    }

    // 2. Draw the jar border (fuchsia) and interior honey gradient in a single pass
    // Use a brighter fuchsia border and specified top honey color for gradient
    drawJarWithProgress(q, r1Rows[q], strips[q].Color(255, 120, 255), 240, 240, 150); // Brighter fuchsia border, top honey (240,240,150)
  }
}

// Reset Round 1 scores
void round1Reset() {
  for (int i = 0; i < 4; i++) {
    r1Rows[i] = 0;
  }
  Serial.println("Round 1: Scores Reset");
}