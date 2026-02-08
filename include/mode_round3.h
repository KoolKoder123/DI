#pragma once
#include "leds.h"
#include "game_state.h"

// Round 3: top quadrants are solid colors + random sparkle flashes.
// Remote buttons (mapped in remote.h) can also shift columns between colors.

// Per-column color state: 0 = BLUE, 1 = GREEN
static uint8_t r3TopLeftColumnColor[QUAD_COLS] = {0};
static uint8_t r3TopRightColumnColor[QUAD_COLS] = {0};

// Random flash settings
static const unsigned long R3_FLASH_TICK_MS = 100;       // how often we attempt new flashes
static const unsigned long R3_FLASH_DURATION_MS = 300;   // flash length
static const int R3_FLASH_ATTEMPTS_PER_TICK = 30;        // random candidates per tick

// Flattened arrays indexed by (q * LEDS_PER_QUAD + physIndex)
static bool r3FlashActive[NUM_STRIPS_CONNECTED * LEDS_PER_QUAD] = {false};
static uint32_t r3FlashSavedColor[NUM_STRIPS_CONNECTED * LEDS_PER_QUAD] = {0};
static unsigned long r3FlashEndTime[NUM_STRIPS_CONNECTED * LEDS_PER_QUAD] = {0};
static unsigned long r3NextFlashTick = 0;

static inline void r3ClearFlashState() {
  for (int i = 0; i < NUM_STRIPS_CONNECTED * LEDS_PER_QUAD; i++) {
    r3FlashActive[i] = false;
    r3FlashSavedColor[i] = 0;
    r3FlashEndTime[i] = 0;
  }
  r3NextFlashTick = 0;
}

static inline void r3FlashTryStart() {
  if (millis() < r3NextFlashTick) return;
  r3NextFlashTick = millis() + R3_FLASH_TICK_MS;

  for (int a = 0; a < R3_FLASH_ATTEMPTS_PER_TICK; a++) {
    // Choose top-left or top-right
    int q = (random(0, 2) == 0) ? Q_TOP_LEFT : Q_TOP_RIGHT;

    // Choose a coordinate inside the visible 18x18 area
    int x = random(0, QUAD_COLS);
    int y = random(0, QUAD_ROWS);
    uint16_t physIdx = xyToIndex(x, y);
    int flat = q * LEDS_PER_QUAD + physIdx;

    if (r3FlashActive[flat]) continue; // already flashing

    // 1/8 chance to start a flash for this candidate
    if (random(8) != 0) continue;

    // Save current color and start flash with a random color
    uint32_t cur = strips[q].getPixelColor(physIdx);
    r3FlashSavedColor[flat] = cur;

    uint8_t r = random(0, 256);
    uint8_t g = random(0, 256);
    uint8_t b = random(0, 256);
    uint32_t newc = strips[q].Color(r, g, b);

    strips[q].setPixelColor(physIdx, newc);
    r3FlashActive[flat] = true;
    r3FlashEndTime[flat] = millis() + R3_FLASH_DURATION_MS;
  }
}

static inline void r3FlashUpdate() {
  bool dirty[NUM_STRIPS_CONNECTED] = {false, false, false, false};
  unsigned long now = millis();

  // Only top quadrants flash.
  for (int q = Q_TOP_LEFT; q <= Q_TOP_RIGHT; q++) {
    for (uint16_t physIdx = 0; physIdx < LEDS_PER_QUAD; physIdx++) {
      int flat = q * LEDS_PER_QUAD + physIdx;
      if (!r3FlashActive[flat]) continue;
      if (now < r3FlashEndTime[flat]) continue;

      // Restore saved color
      strips[q].setPixelColor(physIdx, r3FlashSavedColor[flat]);
      r3FlashActive[flat] = false;
      r3FlashSavedColor[flat] = 0;
      r3FlashEndTime[flat] = 0;
      dirty[q] = true;
    }
  }

  // Push updates for quadrants that changed
  for (int q = Q_TOP_LEFT; q <= Q_TOP_RIGHT; q++) {
    if (dirty[q]) strips[q].show();
  }
}

static inline void r3ClearFlashForColumn(uint8_t q, int x) {
  // Prevent a pending flash restore from overwriting a column color change.
  for (int y = 0; y < QUAD_ROWS; y++) {
    uint16_t physIdx = xyToIndex(x, y);
    int flat = q * LEDS_PER_QUAD + physIdx;
    r3FlashActive[flat] = false;
    r3FlashSavedColor[flat] = strips[q].getPixelColor(physIdx);
    r3FlashEndTime[flat] = 0;
  }
}

static inline void r3StepGreenToBlue() {
  int ql = Q_TOP_LEFT;
  int qr = Q_TOP_RIGHT;
  uint32_t blue = strips[0].Color(0, 0, 255);

  // Scan top-left fully, left to right
  for (int x = 0; x < QUAD_COLS; x++) {
    if (r3TopLeftColumnColor[x] != 1) continue;

    for (int y = 0; y < QUAD_ROWS; y++) {
      strips[ql].setPixelColor(xyToIndex(x, y), blue);
    }
    r3TopLeftColumnColor[x] = 0;
    r3ClearFlashForColumn(ql, x);
    strips[ql].show();
    return;
  }

  // If nothing converted in top-left, scan top-right
  for (int x = 0; x < QUAD_COLS; x++) {
    if (r3TopRightColumnColor[x] != 1) continue;

    for (int y = 0; y < QUAD_ROWS; y++) {
      strips[qr].setPixelColor(xyToIndex(x, y), blue);
    }
    r3TopRightColumnColor[x] = 0;
    r3ClearFlashForColumn(qr, x);
    strips[qr].show();
    return;
  }
}

static inline void r3StepBlueToGreen() {
  int ql = Q_TOP_LEFT;
  int qr = Q_TOP_RIGHT;
  uint32_t green = strips[0].Color(0, 255, 0);

  // Search top-right first, right to left
  for (int x = QUAD_COLS - 1; x >= 0; x--) {
    if (r3TopRightColumnColor[x] != 0) continue;

    for (int y = 0; y < QUAD_ROWS; y++) {
      strips[qr].setPixelColor(xyToIndex(x, y), green);
    }
    r3TopRightColumnColor[x] = 1;
    r3ClearFlashForColumn(qr, x);
    strips[qr].show();
    return;
  }

  // If nothing converted in top-right, search top-left
  for (int x = QUAD_COLS - 1; x >= 0; x--) {
    if (r3TopLeftColumnColor[x] != 0) continue;

    for (int y = 0; y < QUAD_ROWS; y++) {
      strips[ql].setPixelColor(xyToIndex(x, y), green);
    }
    r3TopLeftColumnColor[x] = 1;
    r3ClearFlashForColumn(ql, x);
    strips[ql].show();
    return;
  }
}

static inline void enterRound3() {
  Serial.println("Round 3: Start");
  ledsAllOff();

  uint32_t blue = strips[0].Color(0, 0, 255);
  uint32_t green = strips[0].Color(0, 255, 0);

  // Top quadrants
  fillQuad(Q_TOP_LEFT, blue);
  fillQuad(Q_TOP_RIGHT, green);

  // Bottom quadrants
  drawRedX(Q_BOTTOM_LEFT);
  drawRedX(Q_BOTTOM_RIGHT);

  // Reset column states
  for (int x = 0; x < QUAD_COLS; x++) {
    r3TopLeftColumnColor[x] = 0;  // blue
    r3TopRightColumnColor[x] = 1; // green
  }

  // Clear any leftover flash state so it can't restore old colors.
  r3ClearFlashState();

  Serial.println("Round 3: Ready");
}

// Update active flashes and restore colors when their duration ends.
static inline void randomFlashUpdate() {
  bool dirty[NUM_STRIPS_CONNECTED] = {false, false, false, false};
  unsigned long now = millis();
  for (int q = Q_TOP_LEFT; q <= Q_TOP_RIGHT; q++) {
    for (uint16_t physIdx = 0; physIdx < LEDS_PER_QUAD; physIdx++) {
      int flat = q * LEDS_PER_QUAD + physIdx;
      if (!randomFlashActive[flat]) continue;
      if (now >= randomFlashEndTime[flat]) {
        // Restore saved color
        strips[q].setPixelColor(physIdx, randomFlashSavedColor[flat]);
        randomFlashActive[flat] = false;
        randomFlashSavedColor[flat] = 0;
        randomFlashEndTime[flat] = 0;
        dirty[q] = true;
      }
    }
  }
  // Push updates for quadrants that changed
  for (int q = Q_TOP_LEFT; q <= Q_TOP_RIGHT; q++) if (dirty[q]) strips[q].show();
}