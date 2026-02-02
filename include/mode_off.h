#pragma once
#include "leds.h"

// MODE_OFF: turn everything off.
static inline void enterOff() {
  Serial.println("Mode: OFF");
  ledsAllOff();
}

static inline void runOff(bool canShow) {
  // Nothing to animate in OFF mode.
  // We only clear LEDs once on entry.
  (void)canShow;
}
