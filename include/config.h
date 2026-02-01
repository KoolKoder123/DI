#pragma once
#include <Arduino.h>

// --- HARDWARE CONFIGURATION ---
#define NUM_STRIPS_CONNECTED 4   // How many quadrants are physically wired up right now?
#define QUAD_ROWS 18             // Height of the usable (visible) matrix
#define QUAD_COLS 18             // Width of the usable (visible) matrix

// Physical strip layout: the strip is routed in a 19x19 physical grid
// but one LED per row is consumed by the serpentine turn and should
// remain unused by the visible 18x18 matrix. Keep the usable
// dimensions in `QUAD_*` and expose the physical dimensions below.
#define PHYS_ROWS (QUAD_ROWS+1)       // Physical rows on the strip routing
#define PHYS_COLS (QUAD_COLS+1)       // Physical cols on the strip routing

// Number of LEDs per physical quadrant (what the strip actually has)
#define LEDS_PER_QUAD (PHYS_ROWS * PHYS_COLS)

#define BRIGHTNESS 50            // 0 (off) to 255 (blindingly bright)

// --- PINS ---
// Digital pins for the LED Data wires
const uint8_t LED_PINS[4]  = {8, 9, 6, 7};
// Digital pins for the IR Beam Break sensors
const uint8_t BEAM_PINS[4] = {2, 3, 4, 5};
// Pin for the IR Remote Receiver
#define IR_RECEIVER_PIN 11

//Contestant Mapping to Quadrants
enum ContestantQuadrantMapping {
  QUEEN = 0,
  DOCTOR = 1,
  HEIR = 2,
  INFLUENCER = 3
};

// Physical quadrant index aliases (adjust if wiring differs)
// Assumed mapping: 0 = Top-Left, 1 = Top-Right, 2 = Bottom-Right, 3 = Bottom-Left
#define Q_TOP_LEFT 0
#define Q_TOP_RIGHT 1
#define Q_BOTTOM_RIGHT 2
#define Q_BOTTOM_LEFT 3

// --- GAME STATES ---
// The "State Machine" - tells the Arduino which rules to follow right now
enum Mode {
  MODE_OFF,
  MODE_INTRO,
  MODE_R1,
  MODE_R2,
  MODE_R3,
  MODE_R4,
  MODE_FINALE
};

// Convert Mode enum value to human-readable string
inline const char* modeToString(Mode m) {
  switch (m) {
    case MODE_OFF: return "MODE_OFF";
    case MODE_INTRO: return "MODE_INTRO";
    case MODE_R1: return "MODE_R1";
    case MODE_R2: return "MODE_R2";
    case MODE_R3: return "MODE_R3";
    case MODE_R4: return "MODE_R4";
    case MODE_FINALE: return "MODE_FINALE";
    default: return "UNKNOWN_MODE";
  }
}

// Overload accepting integer values (casts to Mode)
inline const char* modeToString(int m) { return modeToString(static_cast<Mode>(m)); }

// Global variable to track the current state
extern Mode currentMode;

// Flickering variables for bear face (per-quadrant)
extern bool flickerActive[NUM_STRIPS_CONNECTED];
extern unsigned long nextToggleTimePerQuad[NUM_STRIPS_CONNECTED];
extern bool bearOnPerQuad[NUM_STRIPS_CONNECTED];
// Per-quadrant steady-on flags (set by CODE_7 + selector or CODE_2 lock)
extern bool steadyActive[NUM_STRIPS_CONNECTED];
// Armed state: CODE_8 arms flicker; CODE_PREV triggers it for a quadrant
extern bool flickerArmed;
// When true CODE_9 armed: select quadrants to flicker at a faster rate
extern bool flickerFastArmed;
// Per-quadrant marker for VERY fast flicker (set by CODE_9 selection)
extern bool flickerFastPerQuad[NUM_STRIPS_CONNECTED];
// Per-quadrant fixed-lose flicker flag (activated by CODE_LOSE)
extern bool flickerLosePerQuad[NUM_STRIPS_CONNECTED];
// (No single-target quadrant variable; per-quadrant arrays are used)
// MODE_R3 state: which columns in top-left have been turned white
extern bool topLeftColumnsWhite[QUAD_COLS];
// MODE_R3 state: which columns in top-right are white (true) or yellow (false)
extern bool topRightColumnsWhite[QUAD_COLS];
// MODE_R3: per-column color state: 0 = BLUE, 1 = GREEN
extern uint8_t topLeftColumnColor[QUAD_COLS];
extern uint8_t topRightColumnColor[QUAD_COLS];
// Random flash arrays (defined in src/main.cpp)
extern bool randomFlashActive[NUM_STRIPS_CONNECTED * LEDS_PER_QUAD];
extern uint32_t randomFlashSavedColor[NUM_STRIPS_CONNECTED * LEDS_PER_QUAD];
extern unsigned long randomFlashEndTime[NUM_STRIPS_CONNECTED * LEDS_PER_QUAD];
// Lose sequence state (defined in src/main.cpp)
extern bool loseSequenceActive[NUM_STRIPS_CONNECTED];
extern int loseSequenceCount[NUM_STRIPS_CONNECTED];
extern unsigned long loseSequenceNextToggle[NUM_STRIPS_CONNECTED];
// Armed state: CODE_7 arms a steady-on action for PREV to trigger
extern bool steadyArmed;
// When true the bottom-left quadrant stays bright red for the duration of MODE_R2
extern bool bottomLeftLocked;