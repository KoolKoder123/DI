#pragma once
#include "leds.h"
#include "game_state.h"

// Round 2: bear face + flicker effects + lose sequence.
//
// Remote buttons (mapped in remote.h) move currentMode into these states:
// - MODE_R2 (normal)
// - MODE_R2_QUEEN_FLICKERING / MODE_R2_DOCTOR_FLICKERING / MODE_R2_INFLUENCER_FLICKERING
// - MODE_R2_FINAL (fast flicker followed by red X)

// Round 2 Final: flicker bottom-right for 2 seconds, then display red X
static bool round2FinalActive = false;
static unsigned long round2FinalStartTime = 0;
static unsigned long round2FinalNextToggle = 0;
static bool round2FinalBearVisible = true;

static uint32_t bearColor = strips[0].Color(15, 8, 0);
static uint32_t white = strips[0].Color(255, 255, 255);

// Cross-quadrant bear animation helper.
// Draws all bear-face pixels shifted `shift` rows downward from their original
// Q_TOP_LEFT positions. Pixels still in [0, QUAD_ROWS-1] go to Q_TOP_LEFT;
// pixels that fall below y=0 wrap into Q_BOTTOM_LEFT at y + QUAD_ROWS
// (Q_TOP_LEFT y=-1 is physically adjacent to Q_BOTTOM_LEFT y=17).
// Does NOT call clear() or show() — callers do that.
static inline void _bearFacePixelsCross(int shift) {
  auto px = [&](int x, int orig_y, uint32_t col) {
    int ny = orig_y - shift;
    if (ny >= 0 && ny < QUAD_ROWS) {
      strips[Q_TOP_LEFT].setPixelColor(xyToIndex(x, ny), col);
    } else if (ny < 0 && ny >= -QUAD_ROWS) {
      strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(x, ny + QUAD_ROWS), col);
    }
  };
  // Eyes
  px(6,11,white); px(10,11,white);
  // Nose
  px(7,8,white); px(8,8,white); px(9,8,white);
  px(7,9,white); px(9,9,white);
  px(8,7,white); px(8,6,white);
  px(7,5,white); px(9,5,white);
  px(6,9,white); px(10,9,white);
  // Left ear
  px(3,13,white); px(4,12,white); px(5,13,white); px(4,13,white); px(4,14,white);
  // Right ear
  px(12,12,white); px(12,13,white); px(13,13,white); px(11,13,white); px(12,14,white);
  // Bottom of face
  for (int x=5; x<=11; x++) px(x, 2, white);
  // Left side
  px(4,3,white); px(3,4,white); px(2,5,white);
  px(1,6,white); px(1,7,white); px(1,8,white); px(1,9,white);
  px(2,10,white); px(2,11,white);
  px(1,12,white); px(1,13,white); px(1,14,white);
  px(2,15,white); px(3,16,white); px(4,16,white); px(5,16,white);
  px(6,15,white); px(7,14,white);
  // Right side
  px(12,3,white); px(13,4,white); px(14,5,white);
  px(15,6,white); px(15,7,white); px(15,8,white); px(15,9,white);
  px(14,10,white); px(14,11,white);
  px(15,12,white); px(15,13,white); px(15,14,white);
  px(14,15,white); px(13,16,white); px(12,16,white); px(11,16,white);
  px(10,15,white); px(9,14,white);
  // Top middle
  px(8,14,white);
  // Extra bearColor
  px(8,9,bearColor); px(8,5,bearColor); px(2,13,bearColor); px(14,13,bearColor);
  // Fill
  for (int a=5; a<12; a++) px(a, 3,bearColor);
  for (int a=4; a<13; a++) px(a, 4,bearColor);
  for (int a=3; a< 7; a++) px(a, 5,bearColor);
  for (int a=10;a<14; a++) px(a, 5,bearColor);
  for (int a=2; a< 8; a++) px(a, 6,bearColor);
  for (int a=9; a<15; a++) px(a, 6,bearColor);
  for (int a=2; a< 8; a++) px(a, 7,bearColor);
  for (int a=9; a<15; a++) px(a, 7,bearColor);
  for (int a=2; a< 7; a++) px(a, 8,bearColor);
  for (int a=10;a<15; a++) px(a, 8,bearColor);
  for (int a=2; a< 6; a++) px(a, 9,bearColor);
  for (int a=11;a<15; a++) px(a, 9,bearColor);
  for (int a=3; a<14; a++) px(a,10,bearColor);
  for (int a=3; a< 6; a++) px(a,11,bearColor);
  for (int a=7; a<10; a++) px(a,11,bearColor);
  for (int a=11;a<14; a++) px(a,11,bearColor);
  for (int a=2; a< 4; a++) px(a,12,bearColor);
  for (int a=5; a<12; a++) px(a,12,bearColor);
  for (int a=13;a<15; a++) px(a,12,bearColor);
  for (int a=6; a<11; a++) px(a,13,bearColor);
  for (int a=2; a< 4; a++) px(a,14,bearColor);
  for (int a=5; a< 7; a++) px(a,14,bearColor);
  for (int a=10;a<12; a++) px(a,14,bearColor);
  for (int a=13;a<15; a++) px(a,14,bearColor);
  for (int a=3; a< 6; a++) px(a,15,bearColor);
  for (int a=11;a<14; a++) px(a,15,bearColor);
}

// Draws the outline of a bear's face in the specified color
static inline void drawBearFace(uint8_t q) {
  if (q >= NUM_STRIPS_CONNECTED) return;
  if (q == Q_BOTTOM_LEFT) return; // locked quadrant stays X
  
  strips[q].clear();
  
  // Define bear face outline coordinates (x, y)
  // Eyes
  strips[q].setPixelColor(xyToIndex(6, 11), white);
  strips[q].setPixelColor(xyToIndex(10, 11), white);
  
  // Nose (triangle)
  strips[q].setPixelColor(xyToIndex(7, 8), white);
  strips[q].setPixelColor(xyToIndex(8, 8), white);
  strips[q].setPixelColor(xyToIndex(9, 8), white);
  strips[q].setPixelColor(xyToIndex(7, 9), white);
  strips[q].setPixelColor(xyToIndex(9, 9), white);
  strips[q].setPixelColor(xyToIndex(8, 7), white);
  strips[q].setPixelColor(xyToIndex(8, 6), white);
  strips[q].setPixelColor(xyToIndex(7, 5), white);
  strips[q].setPixelColor(xyToIndex(9, 5), white);
  strips[q].setPixelColor(xyToIndex(6, 9), white);
  strips[q].setPixelColor(xyToIndex(10, 9), white);

  // Ears
  strips[q].setPixelColor(xyToIndex(3, 13), white);
  strips[q].setPixelColor(xyToIndex(4, 12), white);
  strips[q].setPixelColor(xyToIndex(5, 13), white);
  strips[q].setPixelColor(xyToIndex(4, 13), white);
  strips[q].setPixelColor(xyToIndex(4, 14), white);
  strips[q].setPixelColor(xyToIndex(12, 12), white);
  strips[q].setPixelColor(xyToIndex(12, 13), white);
  strips[q].setPixelColor(xyToIndex(13, 13), white);
  strips[q].setPixelColor(xyToIndex(11, 13), white);
  strips[q].setPixelColor(xyToIndex(12, 14), white);

  // Bottom of face
  strips[q].setPixelColor(xyToIndex(5, 2), white);
  strips[q].setPixelColor(xyToIndex(6, 2), white);
  strips[q].setPixelColor(xyToIndex(7, 2), white);
  strips[q].setPixelColor(xyToIndex(8, 2), white);
  strips[q].setPixelColor(xyToIndex(9, 2), white);
  strips[q].setPixelColor(xyToIndex(10, 2), white);
  strips[q].setPixelColor(xyToIndex(11, 2), white);

  //Left Side
  strips[q].setPixelColor(xyToIndex(4, 3), white);
  strips[q].setPixelColor(xyToIndex(3, 4), white);
  strips[q].setPixelColor(xyToIndex(2, 5), white);
  strips[q].setPixelColor(xyToIndex(1, 6), white);
  strips[q].setPixelColor(xyToIndex(1, 7), white);
  strips[q].setPixelColor(xyToIndex(1, 8), white);
  strips[q].setPixelColor(xyToIndex(1, 9), white);
  strips[q].setPixelColor(xyToIndex(2, 10), white);
  strips[q].setPixelColor(xyToIndex(2, 11), white);
  strips[q].setPixelColor(xyToIndex(1, 12), white);
  strips[q].setPixelColor(xyToIndex(1, 13), white);
  strips[q].setPixelColor(xyToIndex(1, 14), white);
  strips[q].setPixelColor(xyToIndex(2, 15), white);
  strips[q].setPixelColor(xyToIndex(3, 16), white);
  strips[q].setPixelColor(xyToIndex(4, 16), white);
  strips[q].setPixelColor(xyToIndex(5, 16), white);
  strips[q].setPixelColor(xyToIndex(6, 15), white);
  strips[q].setPixelColor(xyToIndex(7, 14), white);

  //Right Side
  strips[q].setPixelColor(xyToIndex(12, 3), white);
  strips[q].setPixelColor(xyToIndex(13, 4), white);
  strips[q].setPixelColor(xyToIndex(14, 5), white);
  strips[q].setPixelColor(xyToIndex(15, 6), white);
  strips[q].setPixelColor(xyToIndex(15, 7), white);
  strips[q].setPixelColor(xyToIndex(15, 8), white);
  strips[q].setPixelColor(xyToIndex(15, 9), white);
  strips[q].setPixelColor(xyToIndex(14, 10), white);
  strips[q].setPixelColor(xyToIndex(14, 11), white);
  strips[q].setPixelColor(xyToIndex(15, 12), white);
  strips[q].setPixelColor(xyToIndex(15, 13), white);
  strips[q].setPixelColor(xyToIndex(15, 14), white);
  strips[q].setPixelColor(xyToIndex(14, 15), white);
  strips[q].setPixelColor(xyToIndex(13, 16), white);
  strips[q].setPixelColor(xyToIndex(12, 16), white);
  strips[q].setPixelColor(xyToIndex(11, 16), white);
  strips[q].setPixelColor(xyToIndex(10, 15), white);
  strips[q].setPixelColor(xyToIndex(9, 14), white);

  //Top Middle
  strips[q].setPixelColor(xyToIndex(8, 14), white);

  //Extra Pixels
  strips[q].setPixelColor(xyToIndex(8, 9), bearColor);
  strips[q].setPixelColor(xyToIndex(8, 5), bearColor);
  strips[q].setPixelColor(xyToIndex(2, 13), bearColor);
  strips[q].setPixelColor(xyToIndex(14, 13), bearColor);

  for(int a = 5; a < 12; a++) {
    strips[q].setPixelColor(xyToIndex(a, 3), bearColor);
  }
  
  for(int a = 4; a < 13; a++) {
    strips[q].setPixelColor(xyToIndex(a, 4), bearColor);
  }

  for(int a = 3; a < 7; a++) {
    strips[q].setPixelColor(xyToIndex(a, 5), bearColor);
  }

  for(int a = 10; a < 14; a++) {
    strips[q].setPixelColor(xyToIndex(a, 5), bearColor);
  }

  for(int a = 2; a < 8; a++) {
    strips[q].setPixelColor(xyToIndex(a, 6), bearColor);
  }

  for(int a = 9; a < 15; a++) {
    strips[q].setPixelColor(xyToIndex(a, 6), bearColor);
  }

  for(int a = 2; a < 8; a++) {
    strips[q].setPixelColor(xyToIndex(a, 7), bearColor);
  }

  for(int a = 9; a < 15; a++) {
    strips[q].setPixelColor(xyToIndex(a, 7), bearColor);
  }

   for(int a = 2; a < 7; a++) {
    strips[q].setPixelColor(xyToIndex(a, 8), bearColor);
  }

   for(int a = 10; a < 15; a++) {
    strips[q].setPixelColor(xyToIndex(a, 8), bearColor);
  }

   for(int a = 2; a < 6; a++) {
    strips[q].setPixelColor(xyToIndex(a, 9), bearColor);
  }
 
   for(int a = 11; a < 15; a++) {
    strips[q].setPixelColor(xyToIndex(a, 9), bearColor);
  }

  for(int a = 3; a < 14; a++) {
    strips[q].setPixelColor(xyToIndex(a, 10), bearColor);
  }
 
   for(int a = 3; a < 6; a++) {
    strips[q].setPixelColor(xyToIndex(a, 11), bearColor);
  }

  for(int a = 7; a < 10; a++) {
    strips[q].setPixelColor(xyToIndex(a, 11), bearColor);
  }

  for(int a = 11; a < 14; a++) {
    strips[q].setPixelColor(xyToIndex(a, 11), bearColor);
  }

  for(int a = 2; a < 4; a++) {
    strips[q].setPixelColor(xyToIndex(a, 12), bearColor);
  }

  for(int a = 5; a < 12; a++) {
    strips[q].setPixelColor(xyToIndex(a, 12), bearColor);
  }

  for(int a = 13; a < 15; a++) {
    strips[q].setPixelColor(xyToIndex(a, 12), bearColor);
  }

  for(int a = 6; a < 11; a++) {
    strips[q].setPixelColor(xyToIndex(a, 13), bearColor);
  }

  for(int a = 2; a < 4; a++) {
    strips[q].setPixelColor(xyToIndex(a, 14), bearColor);
  }

  for(int a = 5; a < 7; a++) {
    strips[q].setPixelColor(xyToIndex(a, 14), bearColor);
  }

  for(int a = 10; a < 12; a++) {
    strips[q].setPixelColor(xyToIndex(a, 14), bearColor);
  }

  for(int a = 13; a < 15; a++) {
    strips[q].setPixelColor(xyToIndex(a, 14), bearColor);
  }


  for(int a = 3; a < 6; a++) {
    strips[q].setPixelColor(xyToIndex(a, 15), bearColor);
  }

  for(int a = 11; a < 14; a++) {
    strips[q].setPixelColor(xyToIndex(a, 15), bearColor);
  }

  strips[q].show();
}

// Flickers the bear in the specified quadrant with a slow random interval (300-600ms).
// Toggles between showing and hiding the bear, respecting the bottom-left lock.
// Uses global state variables: bearOnPerQuad[], nextToggleTimePerQuad[], bottomLeftLocked.
static inline void flickerBear(uint8_t q) {
  if (millis() < nextToggleTimePerQuad[q]) return;

  // Toggle this quadrant's bear state
  bearOnPerQuad[q] = !bearOnPerQuad[q];

  const uint32_t white = strips[0].Color(255, 255, 255);
  const uint32_t bearColor = strips[0].Color(15, 8, 0);

  if (bearOnPerQuad[q]) {
    // If the quadrant is locked (bottom-left), skip drawing bear there
    if (!(q == Q_BOTTOM_LEFT && bottomLeftLocked)) {
      drawBearFace(q);
    }
  } else {
    // Turn off this quadrant unless it's locked to bright red
    if (!(q == Q_BOTTOM_LEFT && bottomLeftLocked)) {
      strips[q].clear();
      strips[q].show();
    }
  }

  // Schedule next toggle with a random interval
  nextToggleTimePerQuad[q] = millis() + random(200, 300);
}

// Resets flickering for the specified quadrant and draws the bear face fixed (no flicker).
// Stops the flicker animation and ensures the bear stays visible.
static inline void resetFlickerBear(uint8_t q) {
  if (q >= NUM_STRIPS_CONNECTED) return;
  nextToggleTimePerQuad[q] = 0;
  bearOnPerQuad[q] = true;
  if (!(q == Q_BOTTOM_LEFT && bottomLeftLocked)) {
    drawBearFace(q);
  }
}

// Final state for Round 2: starts fast flicker on bottom-right for 2 seconds, then displays red X over it.
// Used to mark the influencer (bottom-right) as eliminated.
// This initializes the flicker state; the main loop will handle the timing and completion.
static inline void round2Final() {
  // Reset flickering on top quadrants - they now display steady bears
  resetFlickerBear(Q_TOP_LEFT);
  resetFlickerBear(Q_TOP_RIGHT);
  
  // Initialize fast flicker on bottom-right quadrant
  round2FinalActive = true;
  round2FinalStartTime = millis();
  round2FinalNextToggle = millis();
  round2FinalBearVisible = true;
}

// Update function for round2Final() flicker state.
// Call this from the main loop while in a flickering mode.
// Returns true if still flickering, false when the 2-second flicker is complete and X is displayed.
static inline bool round2FinalUpdate() {
  if (!round2FinalActive) return false;
  
  unsigned long now = millis();
  unsigned long elapsedMs = now - round2FinalStartTime;
  const unsigned long FLICKER_DURATION_MS = 2000;
  const unsigned long TOGGLE_INTERVAL_MS = 50;
  
  // If 2 seconds have elapsed, finish by displaying the X and return false
  if (elapsedMs >= FLICKER_DURATION_MS) {
    round2FinalActive = false;
    drawBearFace(Q_BOTTOM_RIGHT);
    drawRedXOver(Q_BOTTOM_RIGHT);
    return false;
  }
  
  // Continue flickering: toggle every 50ms
  if (now >= round2FinalNextToggle) {
    round2FinalBearVisible = !round2FinalBearVisible;
    
    if (round2FinalBearVisible) {
      drawBearFace(Q_BOTTOM_RIGHT);
    } else {
      strips[Q_BOTTOM_RIGHT].clear();
      strips[Q_BOTTOM_RIGHT].show();
    }
    
    round2FinalNextToggle = now + TOGGLE_INTERVAL_MS;
  }
  
  return true;
}


