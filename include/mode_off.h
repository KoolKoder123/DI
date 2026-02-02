#pragma once

/*
  mode_off.h

  OFF mode behavior:
  - Turn all LEDs off once when we enter MODE_OFF.
*/

#include <IRremote.hpp>

#include "game_state.h"
#include "leds.h"

static inline void runOffMode() {
  // Only run the enter action once, and only when the IR receiver is idle.
  // Updating LEDs while IR is receiving can corrupt remote input.
  if (currentMode != previousMode && IrReceiver.isIdle()) {
    ledsAllOff();
  }
}
