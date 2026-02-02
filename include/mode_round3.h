#pragma once

/*
  mode_round3.h

  MODE_R3 behavior:
  - Top-left starts BLUE.
  - Top-right starts GREEN.
  - Bottom quadrants show a red X.
  - Random "sparkle" flashes happen on the top quadrants.

  Remote buttons (handled in remote.h):
  - NEXT converts a GREEN column to BLUE (left-to-right scan).
  - PREV converts a BLUE column to GREEN (right-to-left scan).
*/

#include <IRremote.hpp>

#include "game_state.h"
#include "leds.h"
#include "effects_random_flashes.h"

static inline void enterRound3Mode() {
  // Clear and set quadrant visuals for MODE_R3
  ledsAllOff();

  // Top-left: blue, Top-right: green
  uint32_t blue = strips[0].Color(0, 0, 255);
  uint32_t green = strips[0].Color(0, 255, 0);
  fillQuad(Q_TOP_LEFT, blue);
  fillQuad(Q_TOP_RIGHT, green);

  // Bottom-left and bottom-right: 3-pixel-wide red X
  drawRedX(Q_BOTTOM_LEFT);
  drawRedX(Q_BOTTOM_RIGHT);

  // Ensure steady state on all quadrants so nothing flickers
  for (int i = 0; i < NUM_STRIPS_CONNECTED; i++) {
    flickerActive[i] = false;
    flickerFastPerQuad[i] = false;
    flickerLosePerQuad[i] = false;
    nextToggleTimePerQuad[i] = 0;
    bearOnPerQuad[i] = true;
    steadyActive[i] = true;
  }

  // Initialize column state tracking
  for (int x = 0; x < QUAD_COLS; x++) {
    topLeftColumnsWhite[x] = false;
    topRightColumnsWhite[x] = false;
    // 0 = BLUE for top-left, 1 = GREEN for top-right
    topLeftColumnColor[x] = 0;
    topRightColumnColor[x] = 1;
  }

  // MODE_R3 does not use bottom-left lock behavior
  bottomLeftLocked = false;
}

static inline void runRound3Mode() {
  if (currentMode != previousMode && IrReceiver.isIdle()) {
    enterRound3Mode();
  }

  // Random transient flashes (independent of CODE_PREV/CODE_NEXT)
  randomFlashTryStart();
  randomFlashUpdate();
}
