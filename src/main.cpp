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

// Flickering variables for bear face
bool isFlickering = false;
unsigned long nextToggleTime = 0;
bool bearOn = true;

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
        for(int q = 0; q < NUM_STRIPS_CONNECTED; q++) {
          drawBearFace(q, strips[0].Color(255,255,255), bearColor);
        }
        isFlickering = false; // reset flickering when entering mode
      }
      if (isFlickering && millis() >= nextToggleTime) {
        bearOn = !bearOn;
        if (bearOn) {
          uint32_t bearColor = strips[0].Color(15, 8, 0);
          for(int q = 0; q < NUM_STRIPS_CONNECTED; q++) {
            drawBearFace(q, strips[0].Color(255,255,255), bearColor);
          }
        } else {
          ledsAllOff();
        }
        nextToggleTime = millis() + random(100, 400);
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