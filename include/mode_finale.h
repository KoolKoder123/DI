#pragma once

/*
  mode_finale.h

  FINALE mode behavior:
  - Run a slower rainbow animation.
  - Only update LEDs when the IR receiver is idle.
*/

#include <IRremote.hpp>

#include "patterns.h"

static inline void runFinaleMode() {
  if (IrReceiver.isIdle()) {
    finaleUpdate();
  }
}
