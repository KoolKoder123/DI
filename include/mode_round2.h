#pragma once

/*
  mode_round2.h

  MODE_R2 behavior:
  - On entry: show a blue gradient, pause, then draw bear faces.
  - During the mode: handle bear flicker and the lose sequence timing.

  NOTE: Round 2 is very timing-sensitive because we are also reading an IR remote.
*/

#include <IRremote.hpp>

#include "game_state.h"
#include "leds.h"
#include "effects_round2_bear_flicker.h"

// One-time setup when entering MODE_R2.
static inline void enterRound2Mode() {
  // Match the original behavior exactly.
  setBlueGradient();
  delay(1000);
  ledsAllOff();

  // Very dark, muddy brown color for bear face
  uint32_t bearColor = strips[0].Color(15, 8, 0);

  // Immediately lock and fill bottom-left quadrant bright red for MODE_R2
  bottomLeftLocked = true;
  steadyActive[Q_BOTTOM_LEFT] = true;
  drawRedX(Q_BOTTOM_LEFT);

  // Draw bear face only on the other quadrants
  for (int q = 0; q < NUM_STRIPS_CONNECTED; q++) {
    if (q == Q_BOTTOM_LEFT) continue;
    drawBearFace(q, strips[0].Color(255, 255, 255), bearColor);
  }

  // Reset per-quadrant flicker state on mode entry
  for (int i = 0; i < NUM_STRIPS_CONNECTED; i++) {
    flickerActive[i] = false;
    nextToggleTimePerQuad[i] = 0;
    bearOnPerQuad[i] = true;
    flickerFastPerQuad[i] = false;
    flickerLosePerQuad[i] = false;
  }

  flickerArmed = false; // Clear any armed state
  // Keep bottomLeftLocked = true so bottom-left stays bright red during MODE_R2
}

// Per-loop behavior while we remain in MODE_R2.
static inline void runRound2Mode() {
  if (currentMode != previousMode && IrReceiver.isIdle()) {
    enterRound2Mode();
  }

  // Per-quadrant flicker handling and lose sequence timing.
  round2UpdateBearEffects();
}
