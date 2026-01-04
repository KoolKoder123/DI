#pragma once
#include "leds.h"

// Variable to track the color wheel position
uint16_t introHue = 0;

void introUpdate() {
  // 1. LOGIC: Spin the color wheel FASTER
  // We increased this from 500 to 3000.
  // Bigger number = Bigger jumps around the color wheel = "Faster" strobe effect
  introHue += 3000;

  // 2. DRAW: Apply the Rainbow to all strips
  for (int q = 0; q < NUM_STRIPS_CONNECTED; q++) {
    strips[q].rainbow(introHue);
    strips[q].show();
  }
}

void finaleUpdate() {
  static uint16_t hue = 0;
  
  // Slower, majestic rainbow for the winner
  hue += 100; 

  for (int q = 0; q < NUM_STRIPS_CONNECTED; q++) {
    strips[q].rainbow(hue);
    strips[q].show();
  }
}