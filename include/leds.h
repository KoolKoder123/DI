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
  if (y % 2 == 0) {
    // Left to Right
    return y * QUAD_COLS + x;
  } else {
    // Right to Left (Zig-Zag)
    return y * QUAD_COLS + (QUAD_COLS - 1 - x);
  }
}

// Turns on 'rows' amount of LEDs from the bottom up.
// Used in Round 1 to show the container filling up.
void drawProgress(uint8_t q, uint8_t rows, uint32_t color) {
  if (q >= NUM_STRIPS_CONNECTED) return;
  
  strips[q].clear(); // Turn off everything first

  // Math: Calculate total LEDs needed to fill 'rows' amount of lines
  int numLeds = rows * QUAD_COLS;
  
  // Safety: Don't try to light up more LEDs than we have
  if (numLeds > LEDS_PER_QUAD) numLeds = LEDS_PER_QUAD;

  for (int i = 0; i < numLeds; i++) {
    strips[q].setPixelColor(i, color);
  }
  strips[q].show();
}

// Fills the whole quadrant with one color
void fillQuad(uint8_t q, uint32_t color) {
  if (q >= NUM_STRIPS_CONNECTED) return;
  strips[q].fill(color, 0, LEDS_PER_QUAD);
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