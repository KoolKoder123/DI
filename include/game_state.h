#pragma once

/*
  game_state.h

  This project is written in a simple, "Arduino C" style:
  - We keep the current game mode in a global variable.
  - We keep small pieces of memory (arrays) to remember what each quadrant is doing.

  Think of these like sticky notes the Arduino keeps between loop() calls.
*/

#include <Arduino.h>
#include "config.h"

// Global variable to track the current state
extern Mode currentMode;
extern Mode previousMode;

// --- Round 2: Bear flicker state (per quadrant) ---
extern bool flickerActive[NUM_STRIPS_CONNECTED];
extern unsigned long nextToggleTimePerQuad[NUM_STRIPS_CONNECTED];
extern bool bearOnPerQuad[NUM_STRIPS_CONNECTED];

// Per-quadrant steady-on flags (set by CODE_7 + selector or CODE_2 lock)
extern bool steadyActive[NUM_STRIPS_CONNECTED];

// Armed state: CODE_8 arms flicker; selector buttons start flicker per quadrant
extern bool flickerArmed;

// Armed state: CODE_9 arms "very fast" flicker
extern bool flickerFastArmed;

// Per-quadrant marker for VERY fast flicker (set by CODE_9 selection)
extern bool flickerFastPerQuad[NUM_STRIPS_CONNECTED];

// Per-quadrant fixed-lose flicker flag (activated by CODE_100)
extern bool flickerLosePerQuad[NUM_STRIPS_CONNECTED];

// --- Round 3: column color tracking ---
// MODE_R3 state: which columns in top-left have been turned white
extern bool topLeftColumnsWhite[QUAD_COLS];
// MODE_R3 state: which columns in top-right are white (true) or yellow (false)
extern bool topRightColumnsWhite[QUAD_COLS];
// MODE_R3: per-column color state: 0 = BLUE, 1 = GREEN
extern uint8_t topLeftColumnColor[QUAD_COLS];
extern uint8_t topRightColumnColor[QUAD_COLS];

// --- Round 3: random flash memory (so we can restore original colors) ---
extern bool randomFlashActive[NUM_STRIPS_CONNECTED * LEDS_PER_QUAD];
extern uint32_t randomFlashSavedColor[NUM_STRIPS_CONNECTED * LEDS_PER_QUAD];
extern unsigned long randomFlashEndTime[NUM_STRIPS_CONNECTED * LEDS_PER_QUAD];

// --- Round 2: lose-sequence state (CODE_100) ---
extern bool loseSequenceActive[NUM_STRIPS_CONNECTED];
extern int loseSequenceCount[NUM_STRIPS_CONNECTED];
extern unsigned long loseSequenceNextToggle[NUM_STRIPS_CONNECTED];

// Armed state: CODE_7 arms a steady-on action for selector buttons to trigger
extern bool steadyArmed;

// When true the bottom-left quadrant stays bright red for the duration of MODE_R2
extern bool bottomLeftLocked;
