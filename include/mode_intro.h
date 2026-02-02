#pragma once

/*
  mode_intro.h

  INTRO mode behavior:
  - Run a fast rainbow strobe on all quadrants.
  - IMPORTANT: Only update LEDs when the IR receiver is idle.
*/

#include <IRremote.hpp>

#include "patterns.h"

static inline void runIntroMode() {
  if (IrReceiver.isIdle()) {
    introUpdate();
  }
}
