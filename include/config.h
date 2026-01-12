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

#define BRIGHTNESS 40            // 0 (off) to 255 (blindingly bright)

// --- PINS ---
// Digital pins for the LED Data wires
const uint8_t LED_PINS[4]  = {6, 7, 8, 9};
// Digital pins for the IR Beam Break sensors
const uint8_t BEAM_PINS[4] = {2, 3, 4, 5};
// Pin for the IR Remote Receiver
#define IR_RECEIVER_PIN 11

// Analog pins for Potentiometers (Knobs) - Used in later rounds
const uint8_t POT_PINS_R2[3] = {A0, A1, A2};
#define POT_PIN_R3 A3

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

// Global variable to track the current state
extern Mode currentMode;