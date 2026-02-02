#pragma once
#include "leds.h"
#include "game_state.h"

// Round 2: bear face + flicker effects + lose sequence.
//
// Remote buttons (mapped in remote.h) move currentMode into these states:
// - MODE_R2 (normal)
// - MODE_R2_STEADY_ARMED / MODE_R2_FLICKER_ARMED / MODE_R2_FLICKER_FAST_ARMED
// - One-shot actions like MODE_R2_FLICKER_Q_TOP_LEFT (then we return to the armed state)

static bool r2FlickerActive[NUM_STRIPS_CONNECTED] = {false, false, false, false};
static unsigned long r2NextToggleMs[NUM_STRIPS_CONNECTED] = {0, 0, 0, 0};
static bool r2BearVisible[NUM_STRIPS_CONNECTED] = {true, true, true, true};

static bool r2SteadyActive[NUM_STRIPS_CONNECTED] = {false, false, false, false};
static bool r2FastFlicker[NUM_STRIPS_CONNECTED] = {false, false, false, false};

static bool r2BottomLeftLocked = false;

// Lose sequence: toggle 10 times at 50ms, then draw a red X over the bear.
static bool r2LoseActive[NUM_STRIPS_CONNECTED] = {false, false, false, false};
static int r2LoseCount[NUM_STRIPS_CONNECTED] = {0, 0, 0, 0};
static unsigned long r2LoseNextMs[NUM_STRIPS_CONNECTED] = {0, 0, 0, 0};

// Remember the last "stable" Round 2 mode so one-shot commands can return to it.
static Mode r2LastStableMode = MODE_R2;

static inline uint32_t r2BearFillColor() {
  // Very dark brown
  return strips[0].Color(15, 8, 0);
}

static inline uint32_t r2BearOutlineColor() {
  return strips[0].Color(255, 255, 255);
}

static inline void r2DrawBear(uint8_t q) {
  if (q >= NUM_STRIPS_CONNECTED) return;
  if (q == Q_BOTTOM_LEFT && r2BottomLeftLocked) return; // locked quadrant stays X
  drawBearFace(q, r2BearOutlineColor(), r2BearFillColor());
}

static inline void r2TurnOffQuad(uint8_t q) {
  if (q >= NUM_STRIPS_CONNECTED) return;
  if (q == Q_BOTTOM_LEFT && r2BottomLeftLocked) return; // locked quadrant stays X
  strips[q].clear();
  strips[q].show();
}

static inline void r2StopFlicker(uint8_t q) {
  if (q >= NUM_STRIPS_CONNECTED) return;
  r2FlickerActive[q] = false;
  r2FastFlicker[q] = false;
  r2NextToggleMs[q] = 0;
}

static inline void r2LockBottomLeft() {
  r2BottomLeftLocked = true;
  r2SteadyActive[Q_BOTTOM_LEFT] = true;
  drawRedX(Q_BOTTOM_LEFT);
}

static inline bool r2IsStableMode(Mode m) {
  return (m == MODE_R2 ||
          m == MODE_R2_STEADY_ARMED ||
          m == MODE_R2_FLICKER_ARMED ||
          m == MODE_R2_FLICKER_FAST_ARMED);
}

static inline bool r2IsOneShotMode(Mode m) {
  switch (m) {
    case MODE_R2_STEADY_Q_TOP_LEFT:
    case MODE_R2_STEADY_Q_TOP_RIGHT:
    case MODE_R2_STEADY_Q_BOTTOM_RIGHT:
    case MODE_R2_STEADY_Q_BOTTOM_LEFT:

    case MODE_R2_FLICKER_Q_TOP_LEFT:
    case MODE_R2_FLICKER_Q_TOP_RIGHT:
    case MODE_R2_FLICKER_Q_BOTTOM_RIGHT:
    case MODE_R2_FLICKER_Q_BOTTOM_LEFT:

    case MODE_R2_FLICKER_FAST_Q_TOP_LEFT:
    case MODE_R2_FLICKER_FAST_Q_TOP_RIGHT:
    case MODE_R2_FLICKER_FAST_Q_BOTTOM_RIGHT:
    case MODE_R2_FLICKER_FAST_Q_BOTTOM_LEFT:

    case MODE_R2_LOCK_BOTTOM_LEFT:
    case MODE_R2_TRIGGER_LOSE_SEQUENCE:
      return true;
    default:
      return false;
  }
}

static inline Mode r2ReturnModeAfterOneShot(Mode oneShot) {
  switch (oneShot) {
    case MODE_R2_STEADY_Q_TOP_LEFT:
    case MODE_R2_STEADY_Q_TOP_RIGHT:
    case MODE_R2_STEADY_Q_BOTTOM_RIGHT:
    case MODE_R2_STEADY_Q_BOTTOM_LEFT:
      return MODE_R2_STEADY_ARMED;

    case MODE_R2_FLICKER_Q_TOP_LEFT:
    case MODE_R2_FLICKER_Q_TOP_RIGHT:
    case MODE_R2_FLICKER_Q_BOTTOM_RIGHT:
    case MODE_R2_FLICKER_Q_BOTTOM_LEFT:
      return MODE_R2_FLICKER_ARMED;

    case MODE_R2_FLICKER_FAST_Q_TOP_LEFT:
    case MODE_R2_FLICKER_FAST_Q_TOP_RIGHT:
    case MODE_R2_FLICKER_FAST_Q_BOTTOM_RIGHT:
    case MODE_R2_FLICKER_FAST_Q_BOTTOM_LEFT:
      return MODE_R2_FLICKER_FAST_ARMED;

    case MODE_R2_LOCK_BOTTOM_LEFT:
    case MODE_R2_TRIGGER_LOSE_SEQUENCE:
      return r2LastStableMode;

    default:
      return MODE_R2;
  }
}

static inline uint8_t r2QuadrantFromOneShot(Mode oneShot) {
  switch (oneShot) {
    case MODE_R2_STEADY_Q_TOP_LEFT:
    case MODE_R2_FLICKER_Q_TOP_LEFT:
    case MODE_R2_FLICKER_FAST_Q_TOP_LEFT:
      return Q_TOP_LEFT;

    case MODE_R2_STEADY_Q_TOP_RIGHT:
    case MODE_R2_FLICKER_Q_TOP_RIGHT:
    case MODE_R2_FLICKER_FAST_Q_TOP_RIGHT:
      return Q_TOP_RIGHT;

    case MODE_R2_STEADY_Q_BOTTOM_RIGHT:
    case MODE_R2_FLICKER_Q_BOTTOM_RIGHT:
    case MODE_R2_FLICKER_FAST_Q_BOTTOM_RIGHT:
      return Q_BOTTOM_RIGHT;

    case MODE_R2_STEADY_Q_BOTTOM_LEFT:
    case MODE_R2_FLICKER_Q_BOTTOM_LEFT:
    case MODE_R2_FLICKER_FAST_Q_BOTTOM_LEFT:
      return Q_BOTTOM_LEFT;

    default:
      return 0;
  }
}

static inline void r2StartLoseSequence(uint8_t q) {
  if (q >= NUM_STRIPS_CONNECTED) return;

  // Stop flicker on other quadrants so the lose sequence stands out.
  for (int i = 0; i < NUM_STRIPS_CONNECTED; i++) {
    if (i == q) continue;
    r2StopFlicker(i);
  }

  r2LoseActive[q] = true;
  r2LoseCount[q] = 0;
  r2LoseNextMs[q] = millis() + 50;

  // Prepare the quadrant: show bear first.
  r2SteadyActive[q] = false;
  r2StopFlicker(q);
  r2BearVisible[q] = true;

  r2DrawBear(q);

  // Small pixel corrections (keep same behavior as the original code)
  uint32_t whiteCol = strips[q].Color(255, 255, 255);
  uint32_t brownCol = r2BearFillColor();
  uint16_t p;
  p = xyToIndex(6, 9);
  if (strips[q].getPixelColor(p) == whiteCol) strips[q].setPixelColor(p, brownCol);
  p = xyToIndex(11, 13);
  if (strips[q].getPixelColor(p) == whiteCol) strips[q].setPixelColor(p, brownCol);
  p = xyToIndex(12, 14);
  if (strips[q].getPixelColor(p) == whiteCol) strips[q].setPixelColor(p, brownCol);
  strips[q].show();

  Serial.println("Round 2: Lose sequence started (bottom-right)");
}

static inline void r2UpdateLoseSequence(bool canShow) {
  if (!canShow) return;

  unsigned long now = millis();

  for (int q = 0; q < NUM_STRIPS_CONNECTED; q++) {
    if (!r2LoseActive[q]) continue;
    if (now < r2LoseNextMs[q]) continue;

    // Toggle visible state
    r2BearVisible[q] = !r2BearVisible[q];

    if (r2BearVisible[q]) {
      r2DrawBear(q);
    } else {
      r2TurnOffQuad(q);
    }

    r2LoseCount[q]++;
    r2LoseNextMs[q] = now + 50;

    // After 10 toggles, finish with bear + red X over it.
    if (r2LoseCount[q] >= 10) {
      r2LoseActive[q] = false;
      r2LoseCount[q] = 0;

      r2DrawBear(q);
      drawRedXOver(q);

      // Stop other effects on this quadrant.
      r2StopFlicker(q);
      r2SteadyActive[q] = false;

      Serial.println("Round 2: Lose sequence complete");
    }
  }
}

static inline void r2UpdateFlicker(bool canShow) {
  if (!canShow) return;

  unsigned long now = millis();

  for (int q = 0; q < NUM_STRIPS_CONNECTED; q++) {
    if (!r2FlickerActive[q]) continue;
    if (r2SteadyActive[q]) continue; // steady quadrants do not flicker
    if (now < r2NextToggleMs[q]) continue;

    r2BearVisible[q] = !r2BearVisible[q];

    if (r2BearVisible[q]) {
      r2DrawBear(q);
    } else {
      r2TurnOffQuad(q);
    }

    // Choose next toggle interval.
    if (r2FastFlicker[q]) {
      r2NextToggleMs[q] = now + random(20, 100);
    } else {
      r2NextToggleMs[q] = now + random(300, 600);
    }
  }
}

static inline void r2ApplyOneShot(Mode oneShot, bool canShow) {
  if (!canShow) return;

  if (oneShot == MODE_R2_LOCK_BOTTOM_LEFT) {
    r2LockBottomLeft();
    Serial.println("Round 2: Bottom-left locked red (X)");
    return;
  }

  if (oneShot == MODE_R2_TRIGGER_LOSE_SEQUENCE) {
    r2StartLoseSequence(Q_BOTTOM_RIGHT);
    return;
  }

  uint8_t q = r2QuadrantFromOneShot(oneShot);

  // Bottom-left stays locked as an X in this round.
  if (q == Q_BOTTOM_LEFT && r2BottomLeftLocked) {
    r2LockBottomLeft();
    return;
  }

  // Steady actions
  if (oneShot == MODE_R2_STEADY_Q_TOP_LEFT ||
      oneShot == MODE_R2_STEADY_Q_TOP_RIGHT ||
      oneShot == MODE_R2_STEADY_Q_BOTTOM_RIGHT ||
      oneShot == MODE_R2_STEADY_Q_BOTTOM_LEFT) {

    r2StopFlicker(q);
    r2SteadyActive[q] = true;
    r2BearVisible[q] = true;
    r2DrawBear(q);

    Serial.print("Round 2: Quadrant ");
    Serial.print(q);
    Serial.println(" set to steady");
    return;
  }

  // Normal flicker actions
  if (oneShot == MODE_R2_FLICKER_Q_TOP_LEFT ||
      oneShot == MODE_R2_FLICKER_Q_TOP_RIGHT ||
      oneShot == MODE_R2_FLICKER_Q_BOTTOM_RIGHT ||
      oneShot == MODE_R2_FLICKER_Q_BOTTOM_LEFT) {

    r2FlickerActive[q] = true;
    r2FastFlicker[q] = false;
    r2SteadyActive[q] = false;
    r2BearVisible[q] = true;
    r2DrawBear(q);
    r2NextToggleMs[q] = millis() + random(100, 400);

    Serial.print("Round 2: Quadrant ");
    Serial.print(q);
    Serial.println(" started flicker");
    return;
  }

  // Fast flicker actions
  if (oneShot == MODE_R2_FLICKER_FAST_Q_TOP_LEFT ||
      oneShot == MODE_R2_FLICKER_FAST_Q_TOP_RIGHT ||
      oneShot == MODE_R2_FLICKER_FAST_Q_BOTTOM_RIGHT ||
      oneShot == MODE_R2_FLICKER_FAST_Q_BOTTOM_LEFT) {

    r2FlickerActive[q] = true;
    r2FastFlicker[q] = true;
    r2SteadyActive[q] = false;
    r2BearVisible[q] = true;
    r2DrawBear(q);
    r2NextToggleMs[q] = millis() + random(20, 80);

    Serial.print("Round 2: Quadrant ");
    Serial.print(q);
    Serial.println(" started FAST flicker");
    return;
  }
}

static inline void enterRound2() {
  Serial.println("Round 2: Start");

  // Quick blue gradient splash, then clear.
  setBlueGradient();
  delay(1000);
  ledsAllOff();

  // Reset all state.
  r2BottomLeftLocked = false;
  for (int i = 0; i < NUM_STRIPS_CONNECTED; i++) {
    r2FlickerActive[i] = false;
    r2NextToggleMs[i] = 0;
    r2BearVisible[i] = true;
    r2SteadyActive[i] = false;
    r2FastFlicker[i] = false;

    r2LoseActive[i] = false;
    r2LoseCount[i] = 0;
    r2LoseNextMs[i] = 0;
  }

  // Bottom-left is locked as a red X in this round.
  r2LockBottomLeft();

  // Draw bear on the other quadrants.
  for (int q = 0; q < NUM_STRIPS_CONNECTED; q++) {
    if (q == Q_BOTTOM_LEFT) continue;
    r2DrawBear(q);
  }

  // Start in the current Round 2 stable mode (or default to MODE_R2).
  r2LastStableMode = MODE_R2;
  if (r2IsStableMode(currentMode)) r2LastStableMode = currentMode;

  Serial.println("Round 2: Ready");
}

static inline void runRound2(bool canShow) {
  // Remember the last stable mode so one-shot commands can return to it.
  if (r2IsStableMode(currentMode)) {
    r2LastStableMode = currentMode;
  }

  // Friendly messages when switching between the stable "armed" states.
  static Mode lastStableMsgMode = MODE_OFF;
  if (r2IsStableMode(currentMode) && currentMode != lastStableMsgMode) {
    if (currentMode == MODE_R2_STEADY_ARMED) {
      Serial.println("Round 2: Steady armed (PREV/NEXT/PAUSE pick quadrants)");
    } else if (currentMode == MODE_R2_FLICKER_ARMED) {
      Serial.println("Round 2: Flicker armed (PREV/NEXT/PAUSE pick quadrants)");
    } else if (currentMode == MODE_R2_FLICKER_FAST_ARMED) {
      Serial.println("Round 2: FAST flicker armed (PREV/NEXT/PAUSE pick quadrants)");
    }
    lastStableMsgMode = currentMode;
  }

  // One-shot commands: do the action once, then return to an armed/stable mode.
  if (r2IsOneShotMode(currentMode)) {
    Mode oneShot = currentMode;

    r2ApplyOneShot(oneShot, canShow);

    if (canShow) {
      currentMode = r2ReturnModeAfterOneShot(oneShot);
    }
  }

  // Background effects.
  r2UpdateLoseSequence(canShow);
  r2UpdateFlicker(canShow);
}
