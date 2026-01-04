#pragma once
#include "leds.h"
#include "beams.h"

// Score tracking: How many rows are filled in each quadrant?
uint8_t r1Rows[4] = {0,0,0,0};

void round1Update() {
  for (int q = 0; q < NUM_STRIPS_CONNECTED; q++) {
    
    // 1. Check if a ball passed through the beam
    if (beamBroken(q)) {
      if (r1Rows[q] < QUAD_ROWS) {
        r1Rows[q]++; // Increase score
        Serial.print("Point for Quad "); Serial.println(q);
      }
    }

    // 2. Update the LEDs to match the score
    // Uses the new function to fill from Bottom -> Up
    drawProgress(q, r1Rows[q], strips[q].Color(255, 180, 0)); // Honey Color
  }
}