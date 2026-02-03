#pragma once
#include "leds.h"

void finaleUpdate() {
  static uint16_t hue = 0;
  
  // Slower, majestic rainbow for the winner
  hue += 100; 

  for (int q = 0; q < NUM_STRIPS_CONNECTED; q++) {
    strips[q].rainbow(hue);
    strips[q].show();
  }
}