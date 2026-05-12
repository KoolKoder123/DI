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

// Resettable state for the spiral intro so the pattern always starts
// from the center when re-entering the intro mode.
static uint16_t introSpiralBaseHue = 0;
// Resettable state for the radial expansion intro so it always begins
// with the innermost ring on mode entry.
static unsigned long introRadialNextTick = 0;
static int introRadialCurrentRing = -1; // -1 means initial state before first draw
static bool introRadialInHold = false;
static uint16_t introRadialBaseHue = 0;

// Public helper to reset intro state when switching modes.
static inline void introReset() {
  introSpiralBaseHue = 0;
  // Reset radial intro state so it starts with inner ring on next run
  introRadialNextTick = 0;
  introRadialCurrentRing = -1;
  introRadialInHold = false;
  introRadialBaseHue = 0;
}

// Query whether the compiled intro style is the spiral vortex. This
// is a small helper so other modules can make decisions when the
// spiral intro is the active animation.
static inline bool introIsSpiral() {
#if (INTRO_STYLE == INTRO_STYLE_SPIRAL_VORTEX)
  return true;
#else
  return false;
#endif
}

// ---------------------------------------------------------------------------
// Pick the intro visualization here
// ---------------------------------------------------------------------------

#define INTRO_STYLE_RAINBOW_STROBE   1
#define INTRO_STYLE_SPIRAL_VORTEX    2
#define INTRO_STYLE_QUAD_STROBE      3
#define INTRO_STYLE_RADIAL_RAINBOW  4

// Change this number to try different intros.
#ifndef INTRO_STYLE
#define INTRO_STYLE INTRO_STYLE_RADIAL_RAINBOW
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
  // Use shared state so it can be reset on mode entry.
  introSpiralBaseHue += 900; // rotation speed

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
        uint16_t hue = introSpiralBaseHue + (uint16_t)sector * 8192 + (uint16_t)ring * 1300;
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
// Intro 4: Radial rainbow expansion from center (per-quadrant)
// ---------------------------------------------------------------------------

static inline void introRadialRainbowUpdate() {
  unsigned long now = millis();
  // For a single radial origin at the intersection of the four quadrants
  // the "inner corner" pixels are at each quadrant's inner edge. The
  // maximum ring is the max distance from that inner corner to a corner
  // of the quadrant (Chebyshev distance).
  const int maxRing = max(QUAD_COLS - 1, QUAD_ROWS - 1);

  if (now < introRadialNextTick) return;

  if (introRadialInHold) {
    // End of hold: start again with center
    introRadialInHold = false;
    introRadialCurrentRing = 0;
    introRadialBaseHue += 3000; // slight hue shift each cycle
    introRadialNextTick = now + 60; // schedule next ring (faster)
  } else {
    // Advance ring (or start at center)
    if (introRadialCurrentRing < 0) introRadialCurrentRing = 0;
    else introRadialCurrentRing++;

    if (introRadialCurrentRing > maxRing) {
      // Turn everything off and enter hold period
      for (int q = 0; q < NUM_STRIPS_CONNECTED; q++) {
        strips[q].clear();
        strips[q].show();
      }
      introRadialInHold = true;
      introRadialCurrentRing = -1;
      introRadialNextTick = now + 0; // hold off briefly
      return;
    }
    introRadialNextTick = now + 60; // 60 ms between rings
  }

  // Draw all rings up to currentRing (cumulative outward)
  for (int q = 0; q < NUM_STRIPS_CONNECTED; q++) {
    strips[q].clear();

    for (int y = 0; y < QUAD_ROWS; y++) {
      for (int x = 0; x < QUAD_COLS; x++) {
        int dx_inner = 0;
        int dy_inner = 0;

        // Map (x,y) in each quadrant into coordinates where (0,0)
        // is the inner-corner pixel adjacent to the global center.
        switch (q) {
          case Q_TOP_LEFT:
            // inner corner is bottom-right of this quadrant
            dx_inner = (QUAD_COLS - 1) - x;
            dy_inner = y - 0;
            break;
          case Q_TOP_RIGHT:
            // inner corner is bottom-left of this quadrant
            dx_inner = x - 0;
            dy_inner = y - 0;
            break;
          case Q_BOTTOM_RIGHT:
            // inner corner is top-left of this quadrant
            dx_inner = x - 0;
            dy_inner = (QUAD_ROWS - 1) - y;
            break;
          case Q_BOTTOM_LEFT:
            // inner corner is top-right of this quadrant
            dx_inner = (QUAD_COLS - 1) - x;
            dy_inner = (QUAD_ROWS - 1) - y;
            break;
        }

        int adx = abs(dx_inner);
        int ady = abs(dy_inner);
        int ring = (adx > ady) ? adx : ady; // Chebyshev distance

        if (ring <= introRadialCurrentRing) {
          // Use a fixed 7-color rainbow palette for rings (R,O,Y,G,B,I,V).
          // All pixels in the same ring share the same color, and rings
          // cycle after violet back to red.
          static const uint32_t rainbow[7] = {
            // Red
            strips[0].Color(255, 0, 0),
            // Orange
            strips[0].Color(255, 127, 0),
            // Yellow
            strips[0].Color(255, 255, 0),
            // Green
            strips[0].Color(0, 255, 0),
            // Blue
            strips[0].Color(0, 0, 255),
            // Indigo (deep blue-violet)
            strips[0].Color(75, 0, 130),
            // Violet
            strips[0].Color(148, 0, 211)
          };

          uint32_t c = rainbow[ring % 7];
          strips[q].setPixelColor(xyToIndex(x, y), c);
        }
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
#elif (INTRO_STYLE == INTRO_STYLE_RADIAL_RAINBOW)
  introRadialRainbowUpdate();
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
