#pragma once
#include <Arduino.h>

// -----------------------------
// Hardware configuration
// -----------------------------
#define NUM_STRIPS_CONNECTED 4   // How many quadrants are physically wired up right now?
#define QUAD_ROWS 18             // Height of the usable (visible) matrix
#define QUAD_COLS 18             // Width of the usable (visible) matrix

// Physical strip layout: the strip is routed in a 19x19 physical grid,
// but one LED per row is used for the serpentine "turn" and is not part
// of the visible 18x18 matrix.
#define PHYS_ROWS (QUAD_ROWS + 1)
#define PHYS_COLS (QUAD_COLS + 1)

// Number of LEDs per physical quadrant (what the strip actually has)
#define LEDS_PER_QUAD (PHYS_ROWS * PHYS_COLS)

#define BRIGHTNESS 50            // 0 (off) to 255 (very bright)

// -----------------------------
// Pins
// -----------------------------
static const uint8_t LED_PINS[4]  = {8, 9, 6, 7};   // Data pins for each quadrant strip
static const uint8_t BEAM_PINS[4] = {2, 3, 4, 5};   // Beam-break sensor pins
#define IR_RECEIVER_PIN 11                         // IR receiver pin

// Contestant Mapping to Quadrants
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

// -----------------------------
// Game state machine (Modes)
// -----------------------------
// Mode tells the Arduino "what rules to follow right now".
// We also use some sub-modes as one-shot commands inside a round.
enum Mode {
  // Top-level modes
  MODE_OFF,
  MODE_INTRO,
  MODE_R1,
  MODE_R2,
  MODE_R3,
  MODE_R4,
  MODE_FINALE,

  // --- Round 2 sub-modes (armed states) ---
  MODE_R2_STEADY_ARMED,
  MODE_R2_FLICKER_ARMED,
  MODE_R2_FLICKER_FAST_ARMED,

  // --- Round 2 one-shot actions (selected quadrant) ---
  MODE_R2_STEADY_Q_TOP_LEFT,
  MODE_R2_STEADY_Q_TOP_RIGHT,
  MODE_R2_STEADY_Q_BOTTOM_RIGHT,
  MODE_R2_STEADY_Q_BOTTOM_LEFT,

  MODE_R2_FLICKER_Q_TOP_LEFT,
  MODE_R2_FLICKER_Q_TOP_RIGHT,
  MODE_R2_FLICKER_Q_BOTTOM_RIGHT,
  MODE_R2_FLICKER_Q_BOTTOM_LEFT,

  MODE_R2_FLICKER_FAST_Q_TOP_LEFT,
  MODE_R2_FLICKER_FAST_Q_TOP_RIGHT,
  MODE_R2_FLICKER_FAST_Q_BOTTOM_RIGHT,
  MODE_R2_FLICKER_FAST_Q_BOTTOM_LEFT,

  // --- Round 2 one-shot commands ---
  MODE_R2_LOCK_BOTTOM_LEFT,
  MODE_R2_TRIGGER_LOSE_SEQUENCE,

  // --- Round 3 one-shot commands ---
  MODE_R3_STEP_GREEN_TO_BLUE,
  MODE_R3_STEP_BLUE_TO_GREEN
};

// Convert Mode enum value to a human-readable string (for Serial debug).
inline const char* modeToString(Mode m) {
  switch (m) {
    case MODE_OFF: return "MODE_OFF";
    case MODE_INTRO: return "MODE_INTRO";
    case MODE_R1: return "MODE_R1";
    case MODE_R2: return "MODE_R2";
    case MODE_R3: return "MODE_R3";
    case MODE_R4: return "MODE_R4";
    case MODE_FINALE: return "MODE_FINALE";

    case MODE_R2_STEADY_ARMED: return "MODE_R2_STEADY_ARMED";
    case MODE_R2_FLICKER_ARMED: return "MODE_R2_FLICKER_ARMED";
    case MODE_R2_FLICKER_FAST_ARMED: return "MODE_R2_FLICKER_FAST_ARMED";

    case MODE_R2_STEADY_Q_TOP_LEFT: return "MODE_R2_STEADY_Q_TOP_LEFT";
    case MODE_R2_STEADY_Q_TOP_RIGHT: return "MODE_R2_STEADY_Q_TOP_RIGHT";
    case MODE_R2_STEADY_Q_BOTTOM_RIGHT: return "MODE_R2_STEADY_Q_BOTTOM_RIGHT";
    case MODE_R2_STEADY_Q_BOTTOM_LEFT: return "MODE_R2_STEADY_Q_BOTTOM_LEFT";

    case MODE_R2_FLICKER_Q_TOP_LEFT: return "MODE_R2_FLICKER_Q_TOP_LEFT";
    case MODE_R2_FLICKER_Q_TOP_RIGHT: return "MODE_R2_FLICKER_Q_TOP_RIGHT";
    case MODE_R2_FLICKER_Q_BOTTOM_RIGHT: return "MODE_R2_FLICKER_Q_BOTTOM_RIGHT";
    case MODE_R2_FLICKER_Q_BOTTOM_LEFT: return "MODE_R2_FLICKER_Q_BOTTOM_LEFT";

    case MODE_R2_FLICKER_FAST_Q_TOP_LEFT: return "MODE_R2_FLICKER_FAST_Q_TOP_LEFT";
    case MODE_R2_FLICKER_FAST_Q_TOP_RIGHT: return "MODE_R2_FLICKER_FAST_Q_TOP_RIGHT";
    case MODE_R2_FLICKER_FAST_Q_BOTTOM_RIGHT: return "MODE_R2_FLICKER_FAST_Q_BOTTOM_RIGHT";
    case MODE_R2_FLICKER_FAST_Q_BOTTOM_LEFT: return "MODE_R2_FLICKER_FAST_Q_BOTTOM_LEFT";

    case MODE_R2_LOCK_BOTTOM_LEFT: return "MODE_R2_LOCK_BOTTOM_LEFT";
    case MODE_R2_TRIGGER_LOSE_SEQUENCE: return "MODE_R2_TRIGGER_LOSE_SEQUENCE";

    case MODE_R3_STEP_GREEN_TO_BLUE: return "MODE_R3_STEP_GREEN_TO_BLUE";
    case MODE_R3_STEP_BLUE_TO_GREEN: return "MODE_R3_STEP_BLUE_TO_GREEN";

    default: return "UNKNOWN_MODE";
  }
}

// Helper functions to group sub-modes into their top-level mode.
inline bool isRound2Mode(Mode m) {
  switch (m) {
    case MODE_R2:
    case MODE_R2_STEADY_ARMED:
    case MODE_R2_FLICKER_ARMED:
    case MODE_R2_FLICKER_FAST_ARMED:
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

inline bool isRound3Mode(Mode m) {
  switch (m) {
    case MODE_R3:
    case MODE_R3_STEP_GREEN_TO_BLUE:
    case MODE_R3_STEP_BLUE_TO_GREEN:
      return true;
    default:
      return false;
  }
}

// Returns the "top-level" mode for a given mode (sub-modes collapse to their round).
inline Mode topLevelMode(Mode m) {
  if (isRound2Mode(m)) return MODE_R2;
  if (isRound3Mode(m)) return MODE_R3;
  return m;
}
