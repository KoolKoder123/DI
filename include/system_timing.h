#pragma once

/*
  system_timing.h

  At the end of each loop() we pause very briefly.

  Why pause at all?
  - It keeps the system stable and avoids flooding the CPU.

  Why sometimes pause LESS?
  - During very-fast flicker (CODE_100 fixed 40ms) we want to poll the IR remote
    more often so we don't miss button presses.
*/

#include <Arduino.h>

#include "game_state.h"
#include "remote.h" // for readRemote()

static inline void applyEndOfLoopDelay() {
  bool anyLoseActive = false;
  for (int i = 0; i < NUM_STRIPS_CONNECTED; i++) {
    if (flickerLosePerQuad[i]) {
      anyLoseActive = true;
      break;
    }
  }

  if (anyLoseActive) {
    // Poll the remote again quickly to pick up user input during fast flicker
    readRemote();
    delay(1);
  } else {
    delay(10);
  }
}
