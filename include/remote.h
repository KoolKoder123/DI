#pragma once
#include <IRremote.hpp>
#include "config.h"

// --- REMOTE CODES ---
// These hex codes match the specific remote control being used
#define CODE_CH_MINUS  0xBA45FF00  
#define CODE_CH_PLUS   0xB847FF00
#define CODE_0  0xE916FF00
#define CODE_1  0xF30CFF00
#define CODE_2  0xE718FF00
#define CODE_3  0xA15EFF00
#define CODE_4  0xF708FF00
#define CODE_5  0xE31CFF00
#define CODE_7  0xBD42FF00
#define CODE_8  0xAD52FF00
#define CODE_9  0xB54AFF00
#define CODE_PREV  0xBB44FF00
#define CODE_NEXT  0xBF40FF00
#define CODE_PAUSE 0xBC43FF00

void remoteBegin() {
  IrReceiver.begin(IR_RECEIVER_PIN, ENABLE_LED_FEEDBACK);
  Serial.println("IR: Remote Receiver Listening...");
}

void readRemote() {
  // 1. Is there a signal?
  if (!IrReceiver.decode()) return;

  delay(100); // Wait 0.1s for the IR signal to fully settle
  
  // 2. Is it a "Repeat" signal? (holding the button down)
  if (IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT) {
    IrReceiver.resume(); // Ignore it and listen for next signal
    return;
  }

  uint32_t code = IrReceiver.decodedIRData.decodedRawData;
  Mode prev = currentMode;

  // 3. Map buttons to Game Modes
  switch (code) {
    case CODE_CH_MINUS: currentMode = MODE_INTRO;  break;
    case CODE_CH_PLUS:  currentMode = MODE_FINALE; break;
    case CODE_0:        currentMode = MODE_OFF;    break;
    case CODE_1:        currentMode = MODE_R1;     break;
    case CODE_2:
      // If already in MODE_R2, use CODE_2 to lock bottom-left quadrant bright red.
      if (currentMode == MODE_R2) {
        bottomLeftLocked = true;
        steadyActive[Q_BOTTOM_LEFT] = true;
        drawRedX(Q_BOTTOM_LEFT);
        Serial.println("Bottom-left quadrant locked bright red (X)");
      } else {
        currentMode = MODE_R2;
      }
      break;
    case CODE_3:        currentMode = MODE_R3;     break;
    case CODE_4:        currentMode = MODE_R4;     break;
    case CODE_5:        currentMode = MODE_FINALE; break;
    case CODE_7:
      if (currentMode == MODE_R2) {
        // Arm a steady-on action: wait for selectors to make quadrants steady.
        // Disable flicker-armed so subsequent selector presses apply steady, not flicker.
        steadyArmed = true;
        flickerArmed = false;
        // Also disable FAST-flicker arm so selectors will apply steady.
        flickerFastArmed = false;
        Serial.println("Steady armed: press selector(s) to set quadrant(s) steady");
      }
      break;
    case CODE_8:
      if (currentMode == MODE_R2) {
        // Arm the flicker; do not start immediately. Wait for CODE_PREV.
        flickerArmed = true;
        // If user chooses normal flicker, cancel any FAST-flicker arm
        flickerFastArmed = false;
        // Also cancel steady-armed so selectors will apply flicker
        steadyArmed = false;
        Serial.println("Flicker armed: press PREV to begin quadrant flicker");
      }
      break;
    case CODE_9:
      if (currentMode == MODE_R2) {
        // Arm the FAST flicker (shorter interval)
        flickerFastArmed = true;
        // Cancel other armed states so selectors trigger FAST-flicker
        flickerArmed = false;
        steadyArmed = false;
        Serial.println("Fast flicker armed: press selector(s) to begin VERY fast quadrant flicker");
      }
      break;
    // When CODE_8 has armed flicker, these keys choose the quadrant to flicker
    case CODE_NEXT:
      if (currentMode == MODE_R2) {
        int idx = Q_TOP_RIGHT;
        if (flickerFastArmed) {
          // Start VERY fast flickering top-right quadrant (additive)
          flickerActive[idx] = true;
          flickerFastPerQuad[idx] = true;
          bearOnPerQuad[idx] = true;
          steadyActive[idx] = false; // stop steady if it was steady
          uint32_t bearColor = strips[0].Color(15, 8, 0);
          drawBearFace(idx, strips[0].Color(255,255,255), bearColor);
          nextToggleTimePerQuad[idx] = millis() + random(20, 80);
        } else if (flickerArmed) {
          // Start flickering top-right quadrant (additive)
          flickerFastPerQuad[idx] = false;
          flickerActive[idx] = true;
          bearOnPerQuad[idx] = true;
          steadyActive[idx] = false; // stop steady if it was steady
          uint32_t bearColor = strips[0].Color(15, 8, 0);
          drawBearFace(idx, strips[0].Color(255,255,255), bearColor);
          nextToggleTimePerQuad[idx] = millis() + random(100, 400);
        } else if (steadyArmed) {
          // Make top-right quadrant steady (stop flicker and draw bear)
          flickerActive[idx] = false;
          flickerFastPerQuad[idx] = false;
          steadyActive[idx] = true;
          bearOnPerQuad[idx] = true;
          uint32_t bearColor = strips[0].Color(15, 8, 0);
          drawBearFace(idx, strips[0].Color(255,255,255), bearColor);
        }
      }
      break;
    case CODE_PAUSE:
      if (currentMode == MODE_R2) {
        int idx = Q_BOTTOM_RIGHT;
        if (flickerFastArmed) {
          // Start VERY fast flickering bottom-right quadrant (additive)
          flickerActive[idx] = true;
          flickerFastPerQuad[idx] = true;
          bearOnPerQuad[idx] = true;
          steadyActive[idx] = false;
          uint32_t bearColor = strips[0].Color(15, 8, 0);
          drawBearFace(idx, strips[0].Color(255,255,255), bearColor);
          nextToggleTimePerQuad[idx] = millis() + random(20, 80);
        } else if (flickerArmed) {
          // Start flickering bottom-right quadrant (additive)
          flickerFastPerQuad[idx] = false;
          flickerActive[idx] = true;
          bearOnPerQuad[idx] = true;
          steadyActive[idx] = false;
          uint32_t bearColor = strips[0].Color(15, 8, 0);
          drawBearFace(idx, strips[0].Color(255,255,255), bearColor);
          nextToggleTimePerQuad[idx] = millis() + random(100, 400);
        } else if (steadyArmed) {
          // Make bottom-right quadrant steady
          flickerActive[idx] = false;
          flickerFastPerQuad[idx] = false;
          steadyActive[idx] = true;
          bearOnPerQuad[idx] = true;
          uint32_t bearColor = strips[0].Color(15, 8, 0);
          drawBearFace(idx, strips[0].Color(255,255,255), bearColor);
        }
      }
      break;
    
    case CODE_PREV:
      if (currentMode == MODE_R2) {
        int idx = Q_TOP_LEFT;
        if (flickerFastArmed) {
          // Start VERY fast flickering top-left quadrant (additive)
          flickerActive[idx] = true;
          flickerFastPerQuad[idx] = true;
          bearOnPerQuad[idx] = true;
          steadyActive[idx] = false;
          uint32_t bearColor = strips[0].Color(15, 8, 0);
          drawBearFace(idx, strips[0].Color(255,255,255), bearColor);
          nextToggleTimePerQuad[idx] = millis() + random(20, 80);
        } else if (flickerArmed) {
          // Start flickering top-left quadrant (additive)
          flickerFastPerQuad[idx] = false;
          flickerActive[idx] = true;
          bearOnPerQuad[idx] = true;
          steadyActive[idx] = false;
          uint32_t bearColor = strips[0].Color(15, 8, 0);
          drawBearFace(idx, strips[0].Color(255,255,255), bearColor);
          nextToggleTimePerQuad[idx] = millis() + random(100, 400);
        } else if (steadyArmed) {
          // Make top-left quadrant steady (not flickering)
          flickerActive[idx] = false;
          flickerFastPerQuad[idx] = false;
          steadyActive[idx] = true;
          bearOnPerQuad[idx] = true;
          uint32_t bearColor = strips[0].Color(15, 8, 0);
          drawBearFace(idx, strips[0].Color(255,255,255), bearColor); // top-left steady
        }
      }
      break;
    default:
      Serial.print("Unknown Key: 0x");
      Serial.println(code, HEX);
      break;
  }

  // 4. Log changes to Serial Monitor for debugging
  if (currentMode != prev) {
    Serial.print(">> Mode Switched: ");
    Serial.println((int)currentMode);
  }

  // 5. Reset receiver to listen again
  IrReceiver.resume();
}