#pragma once
#include "leds.h"

// MODE_INTRO: fast rainbow strobe.
static uint16_t introHue = 0;

static inline void enterIntro() {
  Serial.println("Mode: INTRO");
}

static inline void runIntro(bool canShow) {
  if (!canShow) return;

  // Jump around the color wheel fast to create a strobe-like rainbow.
  introHue += 3000;

  for (int q = 0; q < NUM_STRIPS_CONNECTED; q++) {
    strips[q].rainbow(introHue);
    strips[q].show();
  }
}
