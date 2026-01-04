#pragma once
#include <Arduino.h>

// --- HARDWARE CONFIGURATION ---
#define NUM_STRIPS_CONNECTED 1   // How many quadrants are physically wired up right now?
#define QUAD_ROWS 19             // Height of the matrix
#define QUAD_COLS 19             // Width of the matrix
#define LEDS_PER_QUAD (QUAD_ROWS * QUAD_COLS)

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