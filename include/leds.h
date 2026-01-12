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
    strips[i].show(); // Initialize all pixels to 'off'
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

// Turn off all LED strips and return to initial (cleared) state
void ledsAllOff() {
  for (int i = 0; i < NUM_STRIPS_CONNECTED; i++) {
    strips[i].clear();
    strips[i].show();
  }
  Serial.println("LEDs: All Off");
}