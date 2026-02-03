#pragma once

/*
  mode_intro.h

  INTRO mode is the "game show opening".

  You can experiment with different intro looks by changing the
  compile-time setting below.
*/

#include <Arduino.h>
#include <IRremote.hpp>

#include "leds.h"

// ---------------------------------------------------------------------------
// Pick the intro visualization here
// ---------------------------------------------------------------------------

#define INTRO_STYLE_RAINBOW_STROBE   1
#define INTRO_STYLE_SPIRAL_VORTEX    2
#define INTRO_STYLE_QUAD_STROBE      3

// Change this number to try different intros.
#ifndef INTRO_STYLE
#define INTRO_STYLE INTRO_STYLE_RAINBOW_STROBE
#endif

// ---------------------------------------------------------------------------
// Intro 1: Fast rainbow strobe (original behavior)
// ---------------------------------------------------------------------------

static inline void introRainbowStrobeUpdate() {
  static uint16_t introHue = 0;
  introHue += 3000; // big jump = fast strobe

  for (int q = 0; q < NUM_STRIPS_CONNECTED; q++) {
    strips[q].rainbow(introHue);
    strips[q].show();
  }
}

// ---------------------------------------------------------------------------
// Intro 2: Fast colorful "spiral vortex" on each quadrant
// ---------------------------------------------------------------------------

// Approximate a direction sector (0..7) without trig.
// This is easy to compute and looks like a rotating swirl.
static inline uint8_t introSector8(int dx, int dy) {
  int ax = abs(dx);
  int ay = abs(dy);

  // Mostly horizontal
  if (ay * 2 < ax) return (dx >= 0) ? 0 : 4;
  // Mostly vertical
  if (ax * 2 < ay) return (dy >= 0) ? 2 : 6;

  // Diagonals
  if (dx >= 0 && dy >= 0) return 1; // NE
  if (dx < 0 && dy >= 0)  return 3; // NW
  if (dx < 0 && dy < 0)   return 5; // SW
  return 7; // SE
}

static inline void introSpiralVortexUpdate() {
  static uint16_t baseHue = 0;
  baseHue += 900; // rotation speed

  // Center of the usable 18x18 grid
  const int cx = (QUAD_COLS - 1) / 2; // 8
  const int cy = (QUAD_ROWS - 1) / 2; // 8

  for (int q = 0; q < NUM_STRIPS_CONNECTED; q++) {
    // Clear everything so unused "turn" LEDs don't stay lit.
    strips[q].clear();

    for (int y = 0; y < QUAD_ROWS; y++) {
      for (int x = 0; x < QUAD_COLS; x++) {
        int dx = x - cx;
        int dy = y - cy;

        // "ring" = how far from the center we are (0..8)
        int ring = abs(dx);
        int ady = abs(dy);
        if (ady > ring) ring = ady;

        uint8_t sector = introSector8(dx, dy); // 0..7

        // Build a hue that changes with direction + distance.
        // This creates the spiral/vortex look.
        uint16_t hue = baseHue + (uint16_t)sector * 8192 + (uint16_t)ring * 1300;
        uint32_t c = strips[q].ColorHSV(hue, 255, 255);
        strips[q].setPixelColor(xyToIndex(x, y), c);
      }
    }

    strips[q].show();
  }
}

// ---------------------------------------------------------------------------
// Intro 3: Quad strobe (big, punchy flashes)
// ---------------------------------------------------------------------------

static inline void introQuadStrobeUpdate() {
  static unsigned long nextTick = 0;
  static bool flashOn = false;
  static uint8_t phase = 0;

  unsigned long now = millis();
  if (now < nextTick) return;
  nextTick = now + 70; // strobe speed

  flashOn = !flashOn;
  if (flashOn) phase++;

  // Colors rotate by phase
  uint32_t c0 = strips[0].Color(255, 0, 0);
  uint32_t c1 = strips[0].Color(0, 255, 0);
  uint32_t c2 = strips[0].Color(0, 0, 255);
  uint32_t c3 = strips[0].Color(255, 255, 255);

  for (int q = 0; q < NUM_STRIPS_CONNECTED; q++) {
    if (!flashOn) {
      strips[q].clear();
      strips[q].show();
      continue;
    }

    // Pick a bold color per quadrant, but rotate it each flash.
    uint8_t pick = (phase + q) & 0x03;
    uint32_t col = (pick == 0) ? c0 : (pick == 1) ? c1 : (pick == 2) ? c2 : c3;

    strips[q].clear();
    // Fill usable matrix area
    for (uint8_t y = 0; y < QUAD_ROWS; y++) {
      for (uint8_t x = 0; x < QUAD_COLS; x++) {
        strips[q].setPixelColor(xyToIndex(x, y), col);
      }
    }
    strips[q].show();
  }
}

// ---------------------------------------------------------------------------
// Public entrypoint
// ---------------------------------------------------------------------------

// Update the intro animation (no IR-idle check here).
static inline void introUpdate() {
#if (INTRO_STYLE == INTRO_STYLE_RAINBOW_STROBE)
  introRainbowStrobeUpdate();
#elif (INTRO_STYLE == INTRO_STYLE_SPIRAL_VORTEX)
  introSpiralVortexUpdate();
#elif (INTRO_STYLE == INTRO_STYLE_QUAD_STROBE)
  introQuadStrobeUpdate();
#else
  introRainbowStrobeUpdate();
#endif
}

// Run intro only when IR receiver is idle (helps remote reliability).
static inline void runIntroMode() {
  if (IrReceiver.isIdle()) {
    introUpdate();
  }
}
