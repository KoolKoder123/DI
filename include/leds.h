#pragma once
#include <Adafruit_NeoPixel.h>
#include "config.h"

// Define the 4 LED strips
Adafruit_NeoPixel strips[4] = {
  Adafruit_NeoPixel(LEDS_PER_QUAD, LED_PINS[0], NEO_GRB + NEO_KHZ800),
  Adafruit_NeoPixel(LEDS_PER_QUAD, LED_PINS[1], NEO_GRB + NEO_KHZ800),
  Adafruit_NeoPixel(LEDS_PER_QUAD, LED_PINS[2], NEO_GRB + NEO_KHZ800),
  Adafruit_NeoPixel(LEDS_PER_QUAD, LED_PINS[3], NEO_GRB + NEO_KHZ800)
};

static inline void ledsBegin() {
  for (int i = 0; i < NUM_STRIPS_CONNECTED; i++) {
    strips[i].begin();
    strips[i].setBrightness(BRIGHTNESS);
    strips[i].clear(); // Clear all pixels to 'off' in the buffer
    strips[i].show();  // Now send the cleared buffer to the strip
  }
  Serial.println("LEDs: System Ready");
}

// (Microphone preview helpers removed — CODE_PREVIEW_R3 now lights specific pixels.)

// Helper: Converts X,Y coordinates to the LED index number.
// ASSUMPTION: Pixel 0 is at Bottom-Left.
// Even Rows (0, 2, 4...) run Left -> Right.
// Odd Rows (1, 3, 5...) run Right -> Left.
uint16_t xyToIndex(uint8_t x, uint8_t y) {
  // Safety: out-of-range requests map to 0
  if (x >= QUAD_COLS || y >= QUAD_ROWS) return 0;

  if (y % 2 == 0) {
    // Left to Right: usable columns map to physical cols 0..(QUAD_COLS-1)
    return (uint16_t)y * PHYS_COLS + x;
  } else {
    // Right to Left: usable columns map to physical cols reversed
    // We avoid the physical "turn" LED at column (PHYS_COLS-1), so
    // map usable x=0..(QUAD_COLS-1) to physical columns (PHYS_COLS-2)..0
    return (uint16_t)y * PHYS_COLS + (PHYS_COLS - 2 - x);
  }
}


// Fills the whole quadrant with one color
void fillQuad(uint8_t q, uint32_t color) {
  if (q >= NUM_STRIPS_CONNECTED) return;
  // Only fill the visible usable 18x18 matrix; keep turn LEDs off.
  for (uint8_t y = 0; y < QUAD_ROWS; y++) {
    for (uint8_t x = 0; x < QUAD_COLS; x++) {
      uint16_t idx = xyToIndex(x, y);
      strips[q].setPixelColor(idx, color);
    }
  }
  strips[q].show();
}

// NOTE:
// The "jar filling" drawing used in Round 1 was moved into mode_round1.h.
// That keeps leds.h focused on generic drawing helpers.


// (Jar interior drawing moved into mode_round1.h)

static inline void ledsAllOff() {
  // ATTEMPT 1: Clear the buffer and push
  for(int i=0; i<NUM_STRIPS_CONNECTED; i++) {
    strips[i].clear();
    strips[i].show();
  }

  // CRITICAL DELAY: Give the IR library time to finish its interrupt
  delay(50); 

  // ATTEMPT 2: Force it again (The "Cleanup" pass)
  for(int i=0; i<NUM_STRIPS_CONNECTED; i++) {
    strips[i].clear();
    strips[i].show();
  }
}

// Sets all LEDs to a blue gradient (darker at bottom, brighter at top)
void setBlueGradient() {
  for(int q = 0; q < NUM_STRIPS_CONNECTED; q++) {
    for(int y = 0; y < QUAD_ROWS; y++) {
      for(int x = 0; x < QUAD_COLS; x++) {
        uint8_t brightness = map(y, 0, QUAD_ROWS - 1, 50, 255);
        uint32_t color = strips[q].Color(0, 0, brightness);
        uint16_t idx = xyToIndex(x, y);
        strips[q].setPixelColor(idx, color);
      }
    }
    strips[q].show();
  }
}

// Draw a red 'X' in the quadrant: two diagonal lines 3 LEDs thick
void drawRedX(uint8_t q) {
  if (q >= NUM_STRIPS_CONNECTED) return;
  strips[q].clear();
  for (uint8_t y = 0; y < QUAD_ROWS; y++) {
    for (uint8_t x = 0; x < QUAD_COLS; x++) {
      // Main diagonal: x == y (top-right to bottom-left)
      // Other diagonal: x + y == QUAD_ROWS - 1 (top-left to bottom-right)
      int diag1 = (int)x - (int)y;
      int diag2 = (int)x + (int)y - (QUAD_ROWS - 1);
      if (abs(diag1) <= 1 || abs(diag2) <= 1) {
        uint16_t idx = xyToIndex(x, y);
        strips[q].setPixelColor(idx, strips[q].Color(255, 0, 0));
      }
    }
  }
  strips[q].show();
}

// Draw a red 'X' over the current contents (do not clear first).
// This overwrites any pixels of the bear that intersect the X.
void drawRedXOver(uint8_t q) {
  if (q >= NUM_STRIPS_CONNECTED) return;
  for (uint8_t y = 0; y < QUAD_ROWS; y++) {
    for (uint8_t x = 0; x < QUAD_COLS; x++) {
      int diag1 = (int)x - (int)y;
      int diag2 = (int)x + (int)y - (QUAD_ROWS - 1);
      if (abs(diag1) <= 1 || abs(diag2) <= 1) {
        uint16_t idx = xyToIndex(x, y);
        strips[q].setPixelColor(idx, strips[q].Color(255, 0, 0));
      }
    }
  }
  strips[q].show();
}

// Draw a stylized 5-petal flower centered in the quadrant `q`.
static inline void drawFlowerInQuad(uint8_t q) {
  if (q >= NUM_STRIPS_CONNECTED) return;

  // Clear quadrant first
  strips[q].clear();

  const int cx = (QUAD_COLS - 1) / 2;
  const int cy = (QUAD_ROWS - 1) / 2;

  static uint32_t yellow = strips[0].Color(255, 200, 0);
  static uint32_t purple = strips[0].Color(128, 0, 128);
  static uint32_t green = strips[0].Color(0, 255, 0);

  // Center of flower
  strips[q].setPixelColor(xyToIndex(9, 9), yellow);
  strips[q].setPixelColor(xyToIndex(9, 10), yellow);
  strips[q].setPixelColor(xyToIndex(10, 9), yellow);
  strips[q].setPixelColor(xyToIndex(8, 10), yellow);
  strips[q].setPixelColor(xyToIndex(8, 9), yellow);
  strips[q].setPixelColor(xyToIndex(8, 8), yellow);
  strips[q].setPixelColor(xyToIndex(7, 9), yellow);
  strips[q].setPixelColor(xyToIndex(7, 8), yellow);
  strips[q].setPixelColor(xyToIndex(9, 8), yellow);
  strips[q].setPixelColor(xyToIndex(10, 8), yellow);
  strips[q].setPixelColor(xyToIndex(9, 7), yellow);
  strips[q].setPixelColor(xyToIndex(8, 7), yellow);

  // Petal 1 Outline
  strips[q].setPixelColor(xyToIndex(7, 11), purple);
  strips[q].setPixelColor(xyToIndex(6, 12), purple);
  strips[q].setPixelColor(xyToIndex(6, 13), purple);
  strips[q].setPixelColor(xyToIndex(6, 14), purple);
  strips[q].setPixelColor(xyToIndex(7, 15), purple);
  strips[q].setPixelColor(xyToIndex(8, 16), purple);

  strips[q].setPixelColor(xyToIndex(10, 11), purple);
  strips[q].setPixelColor(xyToIndex(11, 12), purple);
  strips[q].setPixelColor(xyToIndex(11, 13), purple);
  strips[q].setPixelColor(xyToIndex(11, 14), purple);
  strips[q].setPixelColor(xyToIndex(10, 15), purple);
  strips[q].setPixelColor(xyToIndex(9, 16), purple);

  // Petal 2 Outline
  strips[q].setPixelColor(xyToIndex(6, 10), purple);
  strips[q].setPixelColor(xyToIndex(5, 11), purple);
  strips[q].setPixelColor(xyToIndex(4, 11), purple);
  strips[q].setPixelColor(xyToIndex(3, 11), purple);
  strips[q].setPixelColor(xyToIndex(2, 10), purple);
  strips[q].setPixelColor(xyToIndex(1, 9), purple);

  strips[q].setPixelColor(xyToIndex(1, 8), purple);
  strips[q].setPixelColor(xyToIndex(2, 7), purple);
  strips[q].setPixelColor(xyToIndex(5, 11), purple);
  strips[q].setPixelColor(xyToIndex(3, 6), purple);
  strips[q].setPixelColor(xyToIndex(4, 6), purple);
  strips[q].setPixelColor(xyToIndex(5, 6), purple);
  strips[q].setPixelColor(xyToIndex(6, 7), purple);

  //Petal 3 Outline
  strips[q].setPixelColor(xyToIndex(11, 10), purple);
  strips[q].setPixelColor(xyToIndex(12, 11), purple);
  strips[q].setPixelColor(xyToIndex(13, 11), purple);
  strips[q].setPixelColor(xyToIndex(14, 11), purple);
  strips[q].setPixelColor(xyToIndex(15, 10), purple);
  strips[q].setPixelColor(xyToIndex(16, 9), purple);

  strips[q].setPixelColor(xyToIndex(16, 8), purple);
  strips[q].setPixelColor(xyToIndex(15, 7), purple);
  strips[q].setPixelColor(xyToIndex(14, 6), purple);
  strips[q].setPixelColor(xyToIndex(13, 6), purple);
  strips[q].setPixelColor(xyToIndex(12, 6), purple);
  strips[q].setPixelColor(xyToIndex(11, 7), purple);

  //Petal 4 Outline
  strips[q].setPixelColor(xyToIndex(10, 6), purple);
  strips[q].setPixelColor(xyToIndex(11, 5), purple);
  strips[q].setPixelColor(xyToIndex(11, 4), purple);
  strips[q].setPixelColor(xyToIndex(11, 3), purple);
  strips[q].setPixelColor(xyToIndex(10, 2), purple);
  strips[q].setPixelColor(xyToIndex(9, 1), purple);

  strips[q].setPixelColor(xyToIndex(8, 1), purple);
  strips[q].setPixelColor(xyToIndex(7, 2), purple);
  strips[q].setPixelColor(xyToIndex(6, 3), purple);
  strips[q].setPixelColor(xyToIndex(6, 4), purple);
  strips[q].setPixelColor(xyToIndex(6, 5), purple);
  strips[q].setPixelColor(xyToIndex(7, 6), purple);
  
  //Filling in Petal 1
  for(int a = 7; a <= 10; a++) {
    for(int b = 11; b <= 15; b++) {
      strips[q].setPixelColor(xyToIndex(a, b), purple);
    }
  }
  
  //Filling in Petal 2
  for(int a = 2; a <= 6; a++) {
    for(int b = 7; b <= 10; b++) {
      strips[q].setPixelColor(xyToIndex(a, b), purple);
    }
  }

  //Filling in Petal 3
  for(int a = 11; a <= 15; a++) {
    for(int b = 7; b <= 10; b++) {
      strips[q].setPixelColor(xyToIndex(a, b), purple);
    }
  }

  //Filling in Petal 4
  for(int a = 7; a <= 10; a++) {
    for(int b = 2; b <= 6; b++) {
      strips[q].setPixelColor(xyToIndex(a, b), purple);
    }
  }

  //Extras: Fill in the gaps between petals with purple as well
  strips[q].setPixelColor(xyToIndex(7, 7), purple);
  strips[q].setPixelColor(xyToIndex(10, 7), purple);
  strips[q].setPixelColor(xyToIndex(10, 10), purple);
  strips[q].setPixelColor(xyToIndex(7, 10), purple);

  //Stem
  strips[q].setPixelColor(xyToIndex(6, 6), green);
  strips[q].setPixelColor(xyToIndex(5, 5), green);
  strips[q].setPixelColor(xyToIndex(4, 4), green);
  strips[q].setPixelColor(xyToIndex(3, 3), green);
  strips[q].setPixelColor(xyToIndex(2, 2), green);
  strips[q].setPixelColor(xyToIndex(1, 1), green);

  strips[q].setPixelColor(xyToIndex(5, 4), green);
  strips[q].setPixelColor(xyToIndex(4, 3), green);
  strips[q].setPixelColor(xyToIndex(3, 2), green);
  strips[q].setPixelColor(xyToIndex(2, 1), green);
  strips[q].setPixelColor(xyToIndex(1, 0), green);

  strips[q].setPixelColor(xyToIndex(5, 3), green);
  strips[q].setPixelColor(xyToIndex(4, 2), green);
  strips[q].setPixelColor(xyToIndex(3, 1), green);
  strips[q].setPixelColor(xyToIndex(2, 0), green);
  strips[q].setPixelColor(xyToIndex(3, 0), green);

  strips[q].setPixelColor(xyToIndex(5, 2), green);
  strips[q].setPixelColor(xyToIndex(4, 1), green);
  strips[q].setPixelColor(xyToIndex(4, 0), green);

  strips[q].show();
}

// Draw a 12-LED yellow "ball" at the very bottom center of a quadrant.
// The ball occupies a 4x4 block (rows startY..startY+3, cols startX..startX+3)
// with the four corner pixels left off, resulting in 12 LEDs.
static inline void drawYellowBallBottomCenter(uint8_t q) {
  if (q >= NUM_STRIPS_CONNECTED) return;

  // Compute leftmost column so the 4-wide ball is centered horizontally
  const uint8_t startX = (QUAD_COLS - 4) / 2; // e.g., (18-4)/2 = 7
  const uint8_t startY = 0; // very bottom of the quadrant

  uint32_t yellow = strips[q].Color(255, 255, 0);

  // Clear only the area we'll draw into (optional)
  for (uint8_t y = startY; y < startY + 4; y++) {
    for (uint8_t x = startX; x < startX + 4; x++) {
      // Skip the four corners
      bool isCorner = (x == startX || x == startX + 3) && (y == startY || y == startY + 3);
      if (isCorner) continue;
      uint16_t idx = xyToIndex(x, y);
      strips[q].setPixelColor(idx, yellow);
    }
  }
  strips[q].show();
}

// Animate the yellow ball for the Round 1 preview:
// 1) Start at the very bottom center of the top-left quadrant.
// 2) Move up row-by-row until it occupies rows 7..10 (startY == 7).
// 3) Then move right, crossing into the top-right quadrant, until the
//    ball occupies columns 7..10 of the top-right quadrant.
// Movement step delay: 10 ms per row/column.
static inline void animateYellowBallPreviewR1() {
  const uint8_t startXLeft = (QUAD_COLS - 4) / 2; // initial centered X in left quad
  const uint8_t finalStartY = 7; // target startY so ball covers rows 7,8,9,10
  const uint8_t targetStartXRight = (QUAD_COLS - 4) / 2; // final X in right quad (local)

  uint32_t yellow = strips[0].Color(255, 255, 0);

  // Helper lambdas
  auto clearBallAt = [&](int combinedX, int startY) {
    for (int x = combinedX; x < combinedX + 4; x++) {
      for (int y = startY; y < startY + 4; y++) {
        // Clear entire 4x4 area (including corners) to avoid leaving residual pixels
        if (x < 0) continue;
        if (x < QUAD_COLS) {
          strips[Q_TOP_LEFT].setPixelColor(xyToIndex(x, y), 0);
        } else {
          int lx = x - QUAD_COLS;
          strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(lx, y), 0);
        }
      }
    }
  };

  auto drawBallAt = [&](int combinedX, int startY) {
    for (int x = combinedX; x < combinedX + 4; x++) {
      for (int y = startY; y < startY + 4; y++) {
        bool isCorner = ((x - combinedX) == 0 || (x - combinedX) == 3) && ((y - startY) == 0 || (y - startY) == 3);
        if (isCorner) continue;
        if (x < 0) continue;
        if (x < QUAD_COLS) {
          strips[Q_TOP_LEFT].setPixelColor(xyToIndex(x, y), yellow);
        } else {
          int lx = x - QUAD_COLS;
          strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(lx, y), yellow);
        }
      }
    }
  };

  // Track previous position so we can clear it efficiently
  int prevCombinedX = -100;
  int prevStartY = -100;

  // 1) Vertical move: startY from 0 up to finalStartY
  for (int sy = 0; sy <= (int)finalStartY; sy++) {
    int combinedX = startXLeft; // stays in left quadrant during vertical move
    if (prevCombinedX != -100) clearBallAt(prevCombinedX, prevStartY);
    drawBallAt(combinedX, sy);
    // Show updates for both quadrants involved
    strips[Q_TOP_LEFT].show();
    strips[Q_TOP_RIGHT].show();
    delay(10);
    prevCombinedX = combinedX;
    prevStartY = sy;
  }

  // 2) Horizontal move: from left startX to right target (combined coordinate)
  int combinedStart = startXLeft;
  int combinedTarget = QUAD_COLS + targetStartXRight; // e.g., 18 + 7 = 25
  for (int cx = combinedStart; cx <= combinedTarget; cx++) {
    if (prevCombinedX != -100) clearBallAt(prevCombinedX, prevStartY);
    drawBallAt(cx, finalStartY);
    strips[Q_TOP_LEFT].show();
    strips[Q_TOP_RIGHT].show();
    delay(10);
    prevCombinedX = cx;
    prevStartY = finalStartY;
  }

  // leave the ball drawn at its final location


  // 3) Vertical move down from top-right into bottom-right
  // Move the ball down within the top-right quadrant first, then into bottom-right
  int topRightX = combinedTarget; // combined coordinate in top-right
  // Move down in top-right from finalStartY -> 0
  for (int sy = finalStartY - 1; sy >= 0; sy--) {
    clearBallAt(prevCombinedX, prevStartY);
    drawBallAt(topRightX, sy);
    strips[Q_TOP_RIGHT].show();
    delay(10);
    prevCombinedX = topRightX;
    prevStartY = sy;
  }

  // Now move into bottom-right: start at its top row (QUAD_ROWS-1) and move down to targetStartYBottom
  int startBottomY = QUAD_ROWS - 1;
  int targetStartYBottom = 2; // as requested: final ball rows 2,3,4,5 in bottom-right
  int localXRight = combinedTarget - QUAD_COLS; // local x within right quads

  auto clearBallAtBottom = [&](int lx, int sy) {
    for (int x = lx; x < lx + 4; x++) {
      for (int y = sy; y < sy + 4; y++) {
        // Only clear interior pixels (avoid jar border at cols 0..1 and rows 0..1)
        if (x < 2 || x > (QUAD_COLS - 3)) continue;
        if (y < 2) continue;
        // Bounds check: ensure y is inside quadrant to avoid xyToIndex returning 0
        if (y >= QUAD_ROWS) continue;
        strips[Q_BOTTOM_RIGHT].setPixelColor(xyToIndex(x, y), 0);
      }
    }
  };

  auto drawBallAtBottom = [&](int lx, int sy) {
    for (int x = lx; x < lx + 4; x++) {
      for (int y = sy; y < sy + 4; y++) {
        bool isCorner = ((x - lx) == 0 || (x - lx) == 3) && ((y - sy) == 0 || (y - sy) == 3);
        if (isCorner) continue;
        // Only draw inside jar interior (avoid touching the border)
        if (x < 2 || x > (QUAD_COLS - 3)) continue;
        if (y < 2) continue;
        // Bounds check: ensure y is inside quadrant to avoid xyToIndex returning 0
        if (y >= QUAD_ROWS) continue;
        strips[Q_BOTTOM_RIGHT].setPixelColor(xyToIndex(x, y), yellow);
      }
    }
  };

  // Clear the last top-right position
  clearBallAt(prevCombinedX, prevStartY);

  // Ensure top-right quadrant is fully cleared before the ball moves into bottom-right
  strips[Q_TOP_RIGHT].clear();
  strips[Q_TOP_RIGHT].show();

  for (int by = startBottomY; by >= targetStartYBottom; by--) {
    // When moving into bottom, draw in bottom-right at (localXRight, by)
    // Clear previous bottom position if any
    if (prevCombinedX >= QUAD_COLS) {
      // previous was in top-right; already cleared
    } else if (prevCombinedX != -100) {
      clearBallAtBottom(localXRight, prevStartY);
    }
    drawBallAtBottom(localXRight, by);
    strips[Q_BOTTOM_RIGHT].show();
    delay(10);
    prevCombinedX = -1; // mark as bottom now
    prevStartY = by;
  }

  // 4) Spread rows into the jar interior from bottom->top
  // The jar interior columns are 2..(QUAD_COLS-3) inclusive
  int jarStartX = 2;
  int jarEndX = QUAD_COLS - 3; // 15 for 18 cols

  // Note: the jar border should be drawn by the caller (e.g. CODE_PREVIEW_R1)
  // to avoid including mode_round1.h here and creating circular includes.

  // For each ball row from bottom (offset 0) to top (offset 3)
  for (int rowOffset = 0; rowOffset < 4; rowOffset++) {
    int targetY = targetStartYBottom + rowOffset; // 2,3,4,5
    // Fill the jar interior row with yellow
    for (int x = jarStartX; x <= jarEndX; x++) {
      strips[Q_BOTTOM_RIGHT].setPixelColor(xyToIndex(x, targetY), yellow);
    }
    strips[Q_BOTTOM_RIGHT].show();
    delay(10);
  }

  // Done: leave jar filled rows updated
}