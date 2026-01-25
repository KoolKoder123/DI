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

void ledsBegin() {
  for (int i = 0; i < NUM_STRIPS_CONNECTED; i++) {
    strips[i].begin();
    strips[i].setBrightness(BRIGHTNESS);
    strips[i].clear(); // Clear all pixels to 'off' in the buffer
    strips[i].show();  // Now send the cleared buffer to the strip
  }
  Serial.println("LEDs: System Ready");
}

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

// Turns on 'rows' amount of LEDs from the bottom up.
// Used in Round 1 to show the container filling up.
void drawProgress(uint8_t q, uint8_t rows, uint32_t color) {
  if (q >= NUM_STRIPS_CONNECTED) return;
  
  strips[q].clear(); // Turn off everything first

  // Fill `rows` usable lines from the bottom up using xyToIndex to
  // map to physical strip indices (skipping turn LEDs).
  if (rows > QUAD_ROWS) rows = QUAD_ROWS;

  for (uint8_t y = 0; y < rows; y++) {
    for (uint8_t x = 0; x < QUAD_COLS; x++) {
      uint16_t idx = xyToIndex(x, y);
      strips[q].setPixelColor(idx, color);
    }
  }
  strips[q].show();
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

// Draws a jar border with interior fill in a single pass (no flashing)
// Border: Left, Right, and Bottom sides with 2-column thickness (colorable)
// Interior: Filled with a vertical honey gradient from dark (bottom) to bright (top)
void drawJarWithProgress(uint8_t q, uint8_t rows, uint32_t borderColor, uint8_t topR, uint8_t topG, uint8_t topB) {
  if (q >= NUM_STRIPS_CONNECTED) return;
  
  strips[q].clear(); // Clear once at the start
  
  // Left side: First 2 columns, all rows
  for (uint8_t y = 0; y < QUAD_ROWS; y++) {
    for (uint8_t x = 0; x < 2; x++) {
      uint16_t idx = xyToIndex(x, y);
      strips[q].setPixelColor(idx, borderColor);
    }
  }
  
  // Right side: Last 2 columns, all rows
  for (uint8_t y = 0; y < QUAD_ROWS; y++) {
    for (uint8_t x = QUAD_COLS - 2; x < QUAD_COLS; x++) {
      uint16_t idx = xyToIndex(x, y);
      strips[q].setPixelColor(idx, borderColor);
    }
  }
  
  // Bottom side: First 2 rows (rows 0-1, at the bottom), all columns
  for (uint8_t y = 0; y < 2; y++) {
    for (uint8_t x = 0; x < QUAD_COLS; x++) {
      uint16_t idx = xyToIndex(x, y);
      strips[q].setPixelColor(idx, borderColor);
    }
  }

    // Interior fill: Rows 2 and up (above bottom border), columns 2-16 (inside left/right borders)
    int maxInteriorRows = QUAD_ROWS - 2; // number of interior rows available
    if (rows > maxInteriorRows) rows = maxInteriorRows;

    // Use a fixed bottom honey color for all interior rows (no gradient)
    const uint8_t fillR = 128;
    const uint8_t fillG = 128;
    const uint8_t fillB = 0;

    for (uint8_t y = 2; y < 2 + rows; y++) {
      for (uint8_t x = 2; x < QUAD_COLS - 2; x++) {
        uint16_t idx = xyToIndex(x, y);
        strips[q].setPixelColor(idx, strips[q].Color(fillR, fillG, fillB));
      }
    }
  
  strips[q].show(); // Show once at the end
}

// Draws a jar border: Left, Right, and Bottom sides with 2-column thickness
// Border is white, interior is empty for content
void drawJarBorder(uint8_t q, uint32_t borderColor) {
  if (q >= NUM_STRIPS_CONNECTED) return;
  
  strips[q].clear(); // Start fresh
  
  // Left side: First 2 columns, all rows
  for (uint8_t y = 0; y < QUAD_ROWS; y++) {
    for (uint8_t x = 0; x < 2; x++) {
      uint16_t idx = xyToIndex(x, y);
      strips[q].setPixelColor(idx, borderColor);
    }
  }
  
  // Right side: Last 2 columns, all rows
  for (uint8_t y = 0; y < QUAD_ROWS; y++) {
    for (uint8_t x = QUAD_COLS - 2; x < QUAD_COLS; x++) {
      uint16_t idx = xyToIndex(x, y);
      strips[q].setPixelColor(idx, borderColor);
    }
  }
  
  // Bottom side: First 2 rows (rows 0-1, at the bottom), all columns
  for (uint8_t y = 0; y < 2; y++) {
    for (uint8_t x = 0; x < QUAD_COLS; x++) {
      uint16_t idx = xyToIndex(x, y);
      strips[q].setPixelColor(idx, borderColor);
    }
  }
  
  strips[q].show();
}

// Fills the interior of the jar (excluding the border) from bottom up
// This is used for scoring in Round 1
void drawProgressInterior(uint8_t q, uint8_t rows, uint32_t color) {
  if (q >= NUM_STRIPS_CONNECTED) return;
  
  // Only fill the interior: columns 2-16 (excluding 2-column borders on each side)
  // Rows: From row 2 (above bottom border) up to row 17
  int maxInteriorRows = QUAD_ROWS - 2; // Can fill up to row 17
  
  // Limit rows to interior space
  if (rows > maxInteriorRows) rows = maxInteriorRows;
  
  // Light up interior LEDs
  for (uint8_t y = 2; y < 2 + rows; y++) { // Start from row 2 (above bottom border)
    for (uint8_t x = 2; x < QUAD_COLS - 2; x++) { // Columns 2 to 16 (excluding borders)
      uint16_t idx = xyToIndex(x, y);
      strips[q].setPixelColor(idx, color);
    }
  }
  
  strips[q].show();
}

void ledsAllOff() {
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

// Draws the outline of a bear's face in the specified color
void drawBearFace(uint8_t q, uint32_t outlineColor, uint32_t fillColor) {
  if (q >= NUM_STRIPS_CONNECTED) return;
  
  strips[q].clear();
  
  // Define bear face outline coordinates (x, y)
  // Eyes
  strips[q].setPixelColor(xyToIndex(6, 11), outlineColor);
  strips[q].setPixelColor(xyToIndex(10, 11), outlineColor);
  
  // Nose (triangle)
  strips[q].setPixelColor(xyToIndex(7, 8), outlineColor);
  strips[q].setPixelColor(xyToIndex(8, 8), outlineColor);
  strips[q].setPixelColor(xyToIndex(9, 8), outlineColor);
  strips[q].setPixelColor(xyToIndex(7, 9), outlineColor);
  strips[q].setPixelColor(xyToIndex(9, 9), outlineColor);
  strips[q].setPixelColor(xyToIndex(8, 7), outlineColor);
  strips[q].setPixelColor(xyToIndex(8, 6), outlineColor);
  strips[q].setPixelColor(xyToIndex(7, 5), outlineColor);
  strips[q].setPixelColor(xyToIndex(9, 5), outlineColor);
  strips[q].setPixelColor(xyToIndex(6, 9), outlineColor);
  strips[q].setPixelColor(xyToIndex(10, 9), outlineColor);

  // Ears
  strips[q].setPixelColor(xyToIndex(3, 13), outlineColor);
  strips[q].setPixelColor(xyToIndex(4, 12), outlineColor);
  strips[q].setPixelColor(xyToIndex(5, 13), outlineColor);
  strips[q].setPixelColor(xyToIndex(4, 13), outlineColor);
  strips[q].setPixelColor(xyToIndex(4, 14), outlineColor);
  strips[q].setPixelColor(xyToIndex(12, 12), outlineColor);
  strips[q].setPixelColor(xyToIndex(12, 13), outlineColor);
  strips[q].setPixelColor(xyToIndex(13, 13), outlineColor);
  strips[q].setPixelColor(xyToIndex(11, 13), outlineColor);
  strips[q].setPixelColor(xyToIndex(12, 14), outlineColor);

  // Bottom of face
  strips[q].setPixelColor(xyToIndex(5, 2), outlineColor);
  strips[q].setPixelColor(xyToIndex(6, 2), outlineColor);
  strips[q].setPixelColor(xyToIndex(7, 2), outlineColor);
  strips[q].setPixelColor(xyToIndex(8, 2), outlineColor);
  strips[q].setPixelColor(xyToIndex(9, 2), outlineColor);
  strips[q].setPixelColor(xyToIndex(10, 2), outlineColor);
  strips[q].setPixelColor(xyToIndex(11, 2), outlineColor);

  //Left Side
  strips[q].setPixelColor(xyToIndex(4, 3), outlineColor);
  strips[q].setPixelColor(xyToIndex(3, 4), outlineColor);
  strips[q].setPixelColor(xyToIndex(2, 5), outlineColor);
  strips[q].setPixelColor(xyToIndex(1, 6), outlineColor);
  strips[q].setPixelColor(xyToIndex(1, 7), outlineColor);
  strips[q].setPixelColor(xyToIndex(1, 8), outlineColor);
  strips[q].setPixelColor(xyToIndex(1, 9), outlineColor);
  strips[q].setPixelColor(xyToIndex(2, 10), outlineColor);
  strips[q].setPixelColor(xyToIndex(2, 11), outlineColor);
  strips[q].setPixelColor(xyToIndex(1, 12), outlineColor);
  strips[q].setPixelColor(xyToIndex(1, 13), outlineColor);
  strips[q].setPixelColor(xyToIndex(1, 14), outlineColor);
  strips[q].setPixelColor(xyToIndex(2, 15), outlineColor);
  strips[q].setPixelColor(xyToIndex(3, 16), outlineColor);
  strips[q].setPixelColor(xyToIndex(4, 16), outlineColor);
  strips[q].setPixelColor(xyToIndex(5, 16), outlineColor);
  strips[q].setPixelColor(xyToIndex(6, 15), outlineColor);
  strips[q].setPixelColor(xyToIndex(7, 14), outlineColor);


  //Right Side
  strips[q].setPixelColor(xyToIndex(12, 3), outlineColor);
  strips[q].setPixelColor(xyToIndex(13, 4), outlineColor);
  strips[q].setPixelColor(xyToIndex(14, 5), outlineColor);
  strips[q].setPixelColor(xyToIndex(15, 6), outlineColor);
  strips[q].setPixelColor(xyToIndex(15, 7), outlineColor);
  strips[q].setPixelColor(xyToIndex(15, 8), outlineColor);
  strips[q].setPixelColor(xyToIndex(15, 9), outlineColor);
  strips[q].setPixelColor(xyToIndex(14, 10), outlineColor);
  strips[q].setPixelColor(xyToIndex(14, 11), outlineColor);
  strips[q].setPixelColor(xyToIndex(15, 12), outlineColor);
  strips[q].setPixelColor(xyToIndex(15, 13), outlineColor);
  strips[q].setPixelColor(xyToIndex(15, 14), outlineColor);
  strips[q].setPixelColor(xyToIndex(14, 15), outlineColor);
  strips[q].setPixelColor(xyToIndex(13, 16), outlineColor);
  strips[q].setPixelColor(xyToIndex(12, 16), outlineColor);
  strips[q].setPixelColor(xyToIndex(11, 16), outlineColor);
  strips[q].setPixelColor(xyToIndex(10, 15), outlineColor);
  strips[q].setPixelColor(xyToIndex(9, 14), outlineColor);

  //Top Middle
  strips[q].setPixelColor(xyToIndex(8, 14), outlineColor);

  //Extra Pixels
  strips[q].setPixelColor(xyToIndex(8, 9), fillColor);
  strips[q].setPixelColor(xyToIndex(8, 5), fillColor);
  strips[q].setPixelColor(xyToIndex(2, 13), fillColor);
  strips[q].setPixelColor(xyToIndex(14, 13), fillColor);


  for(int a = 5; a < 12; a++) {
    strips[q].setPixelColor(xyToIndex(a, 3), fillColor);
  }
  
  for(int a = 4; a < 13; a++) {
    strips[q].setPixelColor(xyToIndex(a, 4), fillColor);
  }

  for(int a = 3; a < 7; a++) {
    strips[q].setPixelColor(xyToIndex(a, 5), fillColor);
  }

  for(int a = 10; a < 14; a++) {
    strips[q].setPixelColor(xyToIndex(a, 5), fillColor);
  }

  for(int a = 2; a < 8; a++) {
    strips[q].setPixelColor(xyToIndex(a, 6), fillColor);
  }

  for(int a = 9; a < 15; a++) {
    strips[q].setPixelColor(xyToIndex(a, 6), fillColor);
  }

  for(int a = 2; a < 8; a++) {
    strips[q].setPixelColor(xyToIndex(a, 7), fillColor);
  }

  for(int a = 9; a < 15; a++) {
    strips[q].setPixelColor(xyToIndex(a, 7), fillColor);
  }

   for(int a = 2; a < 7; a++) {
    strips[q].setPixelColor(xyToIndex(a, 8), fillColor);
  }

   for(int a = 10; a < 15; a++) {
    strips[q].setPixelColor(xyToIndex(a, 8), fillColor);
  }

   for(int a = 2; a < 6; a++) {
    strips[q].setPixelColor(xyToIndex(a, 9), fillColor);
  }
 
   for(int a = 11; a < 15; a++) {
    strips[q].setPixelColor(xyToIndex(a, 9), fillColor);
  }

  for(int a = 3; a < 14; a++) {
    strips[q].setPixelColor(xyToIndex(a, 10), fillColor);
  }
 
   for(int a = 3; a < 6; a++) {
    strips[q].setPixelColor(xyToIndex(a, 11), fillColor);
  }

  for(int a = 7; a < 10; a++) {
    strips[q].setPixelColor(xyToIndex(a, 11), fillColor);
  }

  for(int a = 11; a < 14; a++) {
    strips[q].setPixelColor(xyToIndex(a, 11), fillColor);
  }

  for(int a = 2; a < 4; a++) {
    strips[q].setPixelColor(xyToIndex(a, 12), fillColor);
  }

  for(int a = 5; a < 12; a++) {
    strips[q].setPixelColor(xyToIndex(a, 12), fillColor);
  }

  for(int a = 13; a < 15; a++) {
    strips[q].setPixelColor(xyToIndex(a, 12), fillColor);
  }

  for(int a = 6; a < 11; a++) {
    strips[q].setPixelColor(xyToIndex(a, 13), fillColor);
  }



  for(int a = 2; a < 4; a++) {
    strips[q].setPixelColor(xyToIndex(a, 14), fillColor);
  }

  for(int a = 5; a < 7; a++) {
    strips[q].setPixelColor(xyToIndex(a, 14), fillColor);
  }

  for(int a = 10; a < 12; a++) {
    strips[q].setPixelColor(xyToIndex(a, 14), fillColor);
  }

  for(int a = 13; a < 15; a++) {
    strips[q].setPixelColor(xyToIndex(a, 14), fillColor);
  }


  for(int a = 3; a < 6; a++) {
    strips[q].setPixelColor(xyToIndex(a, 15), fillColor);
  }

  for(int a = 11; a < 14; a++) {
    strips[q].setPixelColor(xyToIndex(a, 15), fillColor);
  }
  strips[q].show();
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