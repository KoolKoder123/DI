#include "config.h"
#include "leds.h"
#include "beams.h"
#include "remote.h"
#include "rounds.h"
#include "patterns.h"

// Start the system in OFF mode
Mode currentMode = MODE_OFF;
// Remember the previous mode so we can run enter-actions once
Mode previousMode = MODE_OFF;

// Flickering variables (per-quadrant)
bool flickerActive[NUM_STRIPS_CONNECTED] = {false, false, false, false};
unsigned long nextToggleTimePerQuad[NUM_STRIPS_CONNECTED] = {0,0,0,0};
bool bearOnPerQuad[NUM_STRIPS_CONNECTED] = {true, true, true, true};
// Arm state used to select which quadrant to start flickering
bool flickerArmed = false;
// Fast-arm for CODE_9 selections
bool flickerFastArmed = false;
// Per-quadrant very-fast flicker flags
bool flickerFastPerQuad[NUM_STRIPS_CONNECTED] = {false, false, false, false};
// Per-quadrant steady state
bool steadyActive[NUM_STRIPS_CONNECTED] = {false, false, false, false};
// New: steady-armed state for CODE_7 -> CODE_PREV sequence
bool steadyArmed = false;
// Bottom-left lock: CODE_2 makes bottom-left stay bright red during MODE_R2
bool bottomLeftLocked = false;

void setup() {
  Serial.begin(9600); // Open connection to computer
  while (!Serial) delay(10); // Wait for connection

  randomSeed(millis()); // Seed random number generator

  Serial.println("\n--- HIVE MIND SYSTEM START ---");

  // Initialize all hardware modules
  ledsBegin();
  beamsBegin();
  remoteBegin();

  // Ensure LEDs are off once at startup
  ledsAllOff();
}

void loop() {
  // 1. Always check the remote first
  readRemote();

  // 2. Run the logic for the current Game Mode
  switch (currentMode) {
    case MODE_OFF:
      // Ensure all LEDs are turned off and reset to initial state
      // Only invoke once when entering MODE_OFF (or when idle after a mode change)
      if (currentMode != previousMode && IrReceiver.isIdle()) {
        ledsAllOff();
      }
      break;
      
    case MODE_INTRO:  
      // IMPORTANT: Only update LEDs if the Remote is NOT talking.
      // If we update LEDs while remote is talking, we break the signal.
      if (IrReceiver.isIdle()) introUpdate(); 
      break;
      
    case MODE_R1:
      if (currentMode != previousMode && IrReceiver.isIdle()) {
        ledsAllOff();
        round1Reset();
        beamsReset();
      }
      if (IrReceiver.isIdle()) round1Update(); 
      break;
   
    case MODE_R2:
      if (currentMode != previousMode && IrReceiver.isIdle()) {
        setBlueGradient();
        delay(1000);
        ledsAllOff();
        // Very dark, muddy brown color for bear face
        uint32_t bearColor = strips[0].Color(15, 8, 0);
        // Immediately lock and fill bottom-left quadrant bright red for MODE_R2
        bottomLeftLocked = true;
        steadyActive[Q_BOTTOM_LEFT] = true;
        drawRedX(Q_BOTTOM_LEFT);
        // Draw bear face only on the other quadrants
        for(int q = 0; q < NUM_STRIPS_CONNECTED; q++) {
          if (q == Q_BOTTOM_LEFT) continue;
          drawBearFace(q, strips[0].Color(255,255,255), bearColor);
        }
        // Reset per-quadrant flicker state on mode entry
        for (int i = 0; i < NUM_STRIPS_CONNECTED; i++) {
          flickerActive[i] = false;
          nextToggleTimePerQuad[i] = 0;
          bearOnPerQuad[i] = true;
          flickerFastPerQuad[i] = false;
        }
        flickerArmed = false; // clear any armed state
        // keep bottomLeftLocked = true so bottom-left stays bright red during MODE_R2
      }
      // Per-quadrant flicker handling: allow multiple quadrants to flicker independently
      for (int q = 0; q < NUM_STRIPS_CONNECTED; q++) {
        if (!flickerActive[q]) continue;
      if (steadyActive[q]) continue; // steady quadrants do not flicker
        if (millis() < nextToggleTimePerQuad[q]) continue;

        // Toggle this quadrant's bear state
        bearOnPerQuad[q] = !bearOnPerQuad[q];

        if (bearOnPerQuad[q]) {
          // If the quadrant is locked (bottom-left), skip drawing bear there
          if (!(q == Q_BOTTOM_LEFT && bottomLeftLocked)) {
            uint32_t bearColor = strips[0].Color(15, 8, 0);
            drawBearFace(q, strips[0].Color(255,255,255), bearColor);
          }
        } else {
          // Turn off this quadrant unless it's locked to bright red
          if (!(q == Q_BOTTOM_LEFT && bottomLeftLocked)) {
            strips[q].clear();
            strips[q].show();
          }
        }

        // Choose next toggle interval based on whether this quadrant was
        // selected for VERY-fast flicker (CODE_9) or normal flicker.
        if (flickerFastPerQuad[q]) {
          nextToggleTimePerQuad[q] = millis() + random(20, 100);
        } else {
          nextToggleTimePerQuad[q] = millis() + random(300, 600);
        }
      }
      // Bear face stays on if not flickering
      break;
      
    case MODE_FINALE: 
      if (IrReceiver.isIdle()) finaleUpdate(); 
      break;
      
    default: 
      // Do nothing in other modes for now
      break;
  }

  delay(50); // Small pause to keep things stable

  // Save the mode for the next loop to detect transitions
  previousMode = currentMode;
}