#pragma once
#include "leds.h"

// MODE_FINALE: slow, majestic rainbow.
static uint16_t finaleHue = 0;

static inline void enterFinale() {
  Serial.println("Mode: FINALE");
}

static inline void runFinale(bool canShow) {
  if (!canShow) return;

  finaleHue += 100;

  for (int q = 0; q < NUM_STRIPS_CONNECTED; q++) {
    strips[q].rainbow(finaleHue);
    strips[q].show();
  }
}
