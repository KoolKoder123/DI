#pragma once

// Increase tolerance for IR mark/space matching to allow more timing jitter.
// This overrides the library default (25%) if not already defined.
#ifndef TOLERANCE_FOR_DECODERS_MARK_OR_SPACE_MATCHING_PERCENT
#define TOLERANCE_FOR_DECODERS_MARK_OR_SPACE_MATCHING_PERCENT 40
#endif

#include <IRremote.hpp>
#include "config.h"
#include "mode_round2.h"

// Some IRremote flag macros differ slightly between versions.
// Provide safe fallbacks so this project stays buildable.
#ifndef IRDATA_FLAGS_WAS_OVERFLOW
#define IRDATA_FLAGS_WAS_OVERFLOW 0
#endif
#ifndef IRDATA_FLAGS_PARITY_FAILED
#define IRDATA_FLAGS_PARITY_FAILED 0
#endif

// --- REMOTE CODES ---
// These hex codes match the specific remote control being used
#define CODE_INTRO_PATTERN  0xFE0188FF  
#define CODE_FINALE   0x9E6188FF
#define CODE_RESET  0xFA0588FF
#define CODE_R1  0xA85788FF
#define CODE_R2  0xA75888FF
#define CODE_R3  0xA65988FF
#define CODE_R4  0x9F6088FF
#define CODE_5  0xE31CFF00
#define CODE_QUEEN_FLICKERING  0xEB1488FF
#define CODE_DOCTOR_FLICKERING  0xEA1588FF
#define CODE_INFLUENCER_FLICKERING 0xAD5288FF

#define CODE_FLICKER_SLOW  0x11111111 // 0xEA1588FF1111 xxxx delete this
#define CODE_FLICKER_FAST  0x11111112 // 0xAD5288FF1111 xxxx delete this
#define CODE_QUEEN_RAP_MORE  0xAF5088FF
#define CODE_DOCTOR_RAP_MORE  0xAE5188FF
#define CODE_PAUSE 0xBC43FF00
#define CODE_R100  0xE619FF00
#define CODE_R200 0xF20DFF00
#define CODE_ROUND_FINAL 0xB84788FF

void remoteBegin() {
  IrReceiver.begin(IR_RECEIVER_PIN, ENABLE_LED_FEEDBACK);
  Serial.println("IR: Remote Receiver Listening...");
}

// Returns true if the raw 32-bit code matches one of our known buttons.
//
// Why this exists:
// NeoPixel .show() temporarily disables interrupts. If an IR frame arrives
// during that window, IRremote can sometimes decode a *partial* / corrupted
// frame. Those corrupted values should be ignored (not treated as real input).
static inline bool isKnownRemoteCode(uint32_t code) {
  switch (code) {
    case CODE_INTRO_PATTERN:
    case CODE_FINALE:
    case CODE_RESET:
    case CODE_R1:

    case CODE_R2:
    case CODE_QUEEN_FLICKERING:
    case CODE_INFLUENCER_FLICKERING:
    case CODE_DOCTOR_FLICKERING:

    case CODE_R3:
    case CODE_R4:
    case CODE_5:
    case CODE_FLICKER_SLOW:
    case CODE_FLICKER_FAST:
    case CODE_QUEEN_RAP_MORE:
    case CODE_DOCTOR_RAP_MORE:
    case CODE_PAUSE:
    case CODE_R100:
    case CODE_R200:
    case CODE_ROUND_FINAL:
      return true;
    default:
      return false;
  }
}

// Set to 1 to print extra information about rejected / noisy IR frames.
#ifndef REMOTE_DEBUG
#define REMOTE_DEBUG 1
#endif

void readRemote() {
  // 1. Is there a signal?
  if (!IrReceiver.decode()) return;

  // 2) Robust decode filtering + debounce.
  //
  // Problem we are fixing:
  // - NeoPixel.show() disables interrupts.
  // - If an IR packet arrives during that time, IRremote can decode garbage.
  // - Garbage was being treated as a real "lastRemoteCode", which then caused:
  //     * "Unknown Key" spam
  //     * needing multiple presses to switch modes
  //
  // Fix:
  // - Only accept frames that look valid AND match a known button code.
  // - Never overwrite "lastGoodCode" with unknown/corrupted values.
  // - Debounce ONLY repeated identical codes; different codes should be
  //   accepted immediately (so switching rounds feels responsive).
  static uint32_t lastGoodCode = 0;
  static uint32_t lastProcessedCode = 0;
  static unsigned long lastProcessedTime = 0;

  const unsigned long SAME_CODE_DEBOUNCE_MS = 120;
  unsigned long now = millis();

  bool isRepeat = (IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT);
  uint32_t code = 0;

  if (isRepeat) {
    // Repeat frames do not contain a full 32-bit code. Use the last known-good one.
    if (lastGoodCode == 0) {
      IrReceiver.resume();
      return;
    }
    code = lastGoodCode;
  } else {
    // Reject clearly-bad frames.
    // (These fields/macros are provided by IRremote v4.x)
    if (IrReceiver.decodedIRData.protocol == UNKNOWN) {
#if REMOTE_DEBUG
      Serial.println("IR: Ignored UNKNOWN protocol");
#endif
      IrReceiver.resume();
      return;
    }

    if (IrReceiver.decodedIRData.numberOfBits < 32) {
#if REMOTE_DEBUG
      Serial.print("IR: Ignored short frame bits=");
      Serial.println(IrReceiver.decodedIRData.numberOfBits);
#endif
      IrReceiver.resume();
      return;
    }

    // Parity/overflow flags indicate corrupt data.
    if (IrReceiver.decodedIRData.flags & IRDATA_FLAGS_WAS_OVERFLOW) {
#if REMOTE_DEBUG
      Serial.println("IR: Ignored overflow frame");
#endif
      IrReceiver.resume();
      return;
    }
    if (IrReceiver.decodedIRData.flags & IRDATA_FLAGS_PARITY_FAILED) {
#if REMOTE_DEBUG
      Serial.println("IR: Ignored parity-failed frame");
#endif
      IrReceiver.resume();
      return;
    }

    code = IrReceiver.decodedIRData.decodedRawData;

    // Only accept codes we recognize. This is the key part that stops
    // corrupted partial frames from poisoning our input state.
    if (!isKnownRemoteCode(code)) {
#if REMOTE_DEBUG
      Serial.print("IR: Ignored unknown code 0x");
      Serial.println(code, HEX);
#endif
      IrReceiver.resume();
      return;
    }

    lastGoodCode = code;
  }

  // Debounce only repeats of the *same* code.
  if (code == lastProcessedCode && (now - lastProcessedTime) < SAME_CODE_DEBOUNCE_MS) {
    IrReceiver.resume();
    return;
  }
  lastProcessedCode = code;
  lastProcessedTime = now;
  Mode prev = currentMode;

  // 3. Map buttons to Game Modes
  switch (code) {
    case CODE_INTRO_PATTERN: currentMode = MODE_INTRO;  break;
    case CODE_FINALE:  currentMode = MODE_FINALE; break;
    case CODE_RESET:        currentMode = MODE_OFF;    break;
    case CODE_R1:        currentMode = MODE_R1;     break;
    case CODE_R2:
      // If already in MODE_R2, use CODE_R2 to lock bottom-left quadrant bright red.
      if (currentMode == MODE_R2) {
        bottomLeftLocked = true;
        steadyActive[Q_BOTTOM_LEFT] = true;
        drawRedX(Q_BOTTOM_LEFT);
        Serial.println("Bottom-left quadrant locked bright red (X)");
      } else {
        currentMode = MODE_R2;
      }
      break;
    case CODE_R3:        currentMode = MODE_R3;     break;
    case CODE_R4:        currentMode = MODE_R4;     break;
    case CODE_5:        currentMode = MODE_FINALE; break;
    case CODE_QUEEN_FLICKERING:
      if (currentMode == MODE_R2 || currentMode == MODE_R2_DOCTOR_FLICKERING || 
          currentMode == MODE_R2_INFLUENCER_FLICKERING || currentMode == MODE_R2_QUEEN_FLICKERING) {
        currentMode = MODE_R2_QUEEN_FLICKERING;
        Serial.println("Round 2: Queen Flickering mode");
      }
      break;    
    case CODE_DOCTOR_FLICKERING:
      if (currentMode == MODE_R2 || currentMode == MODE_R2_QUEEN_FLICKERING || 
          currentMode == MODE_R2_INFLUENCER_FLICKERING || currentMode == MODE_R2_DOCTOR_FLICKERING) {
        currentMode = MODE_R2_DOCTOR_FLICKERING;
        Serial.println("Round 2: Doctor Flickering mode");
      }
      break;
    case CODE_INFLUENCER_FLICKERING:
      if (currentMode == MODE_R2 || currentMode == MODE_R2_QUEEN_FLICKERING || 
          currentMode == MODE_R2_DOCTOR_FLICKERING || currentMode == MODE_R2_INFLUENCER_FLICKERING) {
        currentMode = MODE_R2_INFLUENCER_FLICKERING;
        Serial.println("Round 2: Influencer Flickering mode");
      }
      break;    
    case CODE_FLICKER_SLOW:
      if (currentMode == MODE_R2) {
        // Arm the flicker; do not start immediately. Wait for CODE_QUEEN_RAP_MORE.
        flickerArmed = true;
        // If user chooses normal flicker, ensure steady is not armed
        // Also cancel steady-armed so selectors will apply flicker
        steadyArmed = false;
        Serial.println("Flicker armed: press PREV to begin quadrant flicker");
      }
      break;
    case CODE_FLICKER_FAST:
      if (currentMode == MODE_R2) {
        // Arm the FAST flicker (shorter interval)
        // NOTE: FAST flicker behavior removed. Fall back to normal flicker arm.
        flickerArmed = true;
        steadyArmed = false;
        Serial.println("Fast flicker removed: arming normal flicker instead");
      }
      break;

    // When CODE_FLICKER_SLOW has armed flicker, these keys choose the quadrant to flicker
    case CODE_DOCTOR_RAP_MORE:
      if (currentMode == MODE_R3) {
        // If Round 3 is in its "lose" blinking state, ignore column edits.
        if (round3BlinkActive) {
          Serial.println("Round 3: blinking - ignoring NEXT");
          break;
        }
        // Randomized conversion: change either 1 or 2 GREEN columns to BLUE.
        // Choose 1 or 2 uniformly.
        int toConvert = random(0, 2) + 1; // 1 or 2
        int convertedCount = 0;
        int ql = Q_TOP_LEFT;
        int qr = Q_TOP_RIGHT;
        uint32_t blue = strips[0].Color(0,0,255);

        // First pass: scan top-left left->right
        for (int x = 0; x < QUAD_COLS && convertedCount < toConvert; x++) {
          if (topLeftColumnColor[x] == 1) {
            for (int y = 0; y < QUAD_ROWS; y++) {
              uint16_t physIdx = xyToIndex(x, y);
              strips[ql].setPixelColor(physIdx, blue);
              int flat = ql * LEDS_PER_QUAD + physIdx;
              randomFlashActive[flat] = false;
              randomFlashSavedColor[flat] = strips[ql].getPixelColor(physIdx);
              randomFlashEndTime[flat] = 0;
            }
            topLeftColumnColor[x] = 0; // now blue
            strips[ql].show();
            convertedCount++;
          }
        }

        // Second pass: if still need conversions, scan top-right left->right
        for (int x = 0; x < QUAD_COLS && convertedCount < toConvert; x++) {
          if (topRightColumnColor[x] == 1) {
            for (int y = 0; y < QUAD_ROWS; y++) {
              uint16_t physIdx = xyToIndex(x, y);
              strips[qr].setPixelColor(physIdx, blue);
              int flat = qr * LEDS_PER_QUAD + physIdx;
              randomFlashActive[flat] = false;
              randomFlashSavedColor[flat] = strips[qr].getPixelColor(physIdx);
              randomFlashEndTime[flat] = 0;
            }
            topRightColumnColor[x] = 0; // now blue
            strips[qr].show();
            convertedCount++;
          }
        }
      } else if (currentMode == MODE_R2) {
        int idx = Q_TOP_RIGHT;
        if (flickerArmed) {
          // Start flickering top-right quadrant (additive)
          flickerActive[idx] = true;
          bearOnPerQuad[idx] = true;
          steadyActive[idx] = false; // stop steady if it was steady          
          drawBearFace(idx);
          nextToggleTimePerQuad[idx] = millis() + random(100, 400);
        } else if (steadyArmed) {
          // Make top-right quadrant steady (stop flicker and draw bear)
          flickerActive[idx] = false;
          steadyActive[idx] = true;
          bearOnPerQuad[idx] = true;          
          drawBearFace(idx);
        }
      }
      break;
    case CODE_PAUSE:
      if (currentMode == MODE_R2) {
        int idx = Q_BOTTOM_RIGHT;
        if (flickerArmed) {
          // Start flickering bottom-right quadrant (additive)
          flickerActive[idx] = true;
          bearOnPerQuad[idx] = true;
          steadyActive[idx] = false;
          drawBearFace(idx);
          nextToggleTimePerQuad[idx] = millis() + random(100, 400);
        } else if (steadyArmed) {
          // Make bottom-right quadrant steady
          flickerActive[idx] = false;
          steadyActive[idx] = true;
          bearOnPerQuad[idx] = true;
          drawBearFace(idx);
        }
      }
      break;
    
    case CODE_QUEEN_RAP_MORE:
      if (currentMode == MODE_R3) {
        // If Round 3 is in its "lose" blinking state, ignore column edits.
        if (round3BlinkActive) {
          Serial.println("Round 3: blinking - ignoring PREV");
          break;
        }
        // Randomized conversion: change either 1 or 2 BLUE columns to GREEN.
        int toConvert = random(0, 2) + 1; // 1 or 2
        int convertedCount = 0;
        int ql = Q_TOP_LEFT;
        int qr = Q_TOP_RIGHT;
        uint32_t green = strips[0].Color(0,255,0);

        // First pass: scan top-right right->left
        for (int x = QUAD_COLS - 1; x >= 0 && convertedCount < toConvert; x--) {
          if (topRightColumnColor[x] == 0) {
            for (int y = 0; y < QUAD_ROWS; y++) {
              uint16_t physIdx = xyToIndex(x, y);
              strips[qr].setPixelColor(physIdx, green);
              int flat = qr * LEDS_PER_QUAD + physIdx;
              randomFlashActive[flat] = false;
              randomFlashSavedColor[flat] = strips[qr].getPixelColor(physIdx);
              randomFlashEndTime[flat] = 0;
            }
            topRightColumnColor[x] = 1; // now green
            strips[qr].show();
            convertedCount++;
          }
        }

        // Second pass: if still need conversions, scan top-left right->left
        for (int x = QUAD_COLS - 1; x >= 0 && convertedCount < toConvert; x--) {
          if (topLeftColumnColor[x] == 0) {
            for (int y = 0; y < QUAD_ROWS; y++) {
              uint16_t physIdx = xyToIndex(x, y);
              strips[ql].setPixelColor(physIdx, green);
              int flat = ql * LEDS_PER_QUAD + physIdx;
              randomFlashActive[flat] = false;
              randomFlashSavedColor[flat] = strips[ql].getPixelColor(physIdx);
              randomFlashEndTime[flat] = 0;
            }
            topLeftColumnColor[x] = 1; // now green
            strips[ql].show();
            convertedCount++;
          }
        }
      } else if (currentMode == MODE_R2) {
        int idx = Q_TOP_LEFT;
        if (flickerArmed) {
          // Start flickering top-left quadrant (additive)
          flickerActive[idx] = true;
          bearOnPerQuad[idx] = true;
          steadyActive[idx] = false;
          uint32_t bearColor = strips[0].Color(15, 8, 0);
          drawBearFace(idx);
          nextToggleTimePerQuad[idx] = millis() + random(100, 400);
        } else if (steadyArmed) {
          // Make top-left quadrant steady (not flickering)
          flickerActive[idx] = false;
          steadyActive[idx] = true;
          bearOnPerQuad[idx] = true;
          uint32_t bearColor = strips[0].Color(15, 8, 0);
          drawBearFace(idx); // top-left steady
        }
      }
      break;

    case CODE_ROUND_FINAL:
      // CODE_ROUND_FINAL is a "lose" shortcut, and does different things per round.
      if (currentMode == MODE_R1) {
        // Round 1: bottom-left loses (red X overlay on current jar).
        round1BottomLeftEliminated = true;
        Serial.println("Round 1: bottom-left eliminated (X)");
      } else if (currentMode == MODE_R2 || currentMode == MODE_R2_QUEEN_FLICKERING || currentMode == MODE_R2_INFLUENCER_FLICKERING || currentMode == MODE_R2_DOCTOR_FLICKERING) {
        currentMode = MODE_R2_FINAL;
        Serial.println("Round 2: Final sequence initiated");
      } else if (currentMode == MODE_R2_FINAL) {
        // Already in final mode; restart the sequence if desired
        Serial.println("Round 2: Final sequence restarted");
      } else if (currentMode == MODE_R3) {
        // Round 3: reset the top quadrants and blink them.
        round3ResetAndBlinkRequested = true;
        round3BlinkActive = true;
        Serial.println("Round 3: reset + blink requested (top quadrants)");
      } else if (currentMode == MODE_R4) {
        // Round 4: eliminate everyone except bottom-left.
        for (int i = 0; i < NUM_STRIPS_CONNECTED; i++) round4Eliminated[i] = false;
        round4Eliminated[Q_TOP_LEFT] = true;
        round4Eliminated[Q_TOP_RIGHT] = true;
        round4Eliminated[Q_BOTTOM_RIGHT] = true;
        round4Eliminated[Q_BOTTOM_LEFT] = false;
        Serial.println("Round 4: eliminated all but bottom-left (X)");
      } else {
        Serial.println("EQ button pressed");
      }
      break;

    case CODE_R200:
      // Reserved for future use.
      Serial.println("200 button pressed");
      break;

    default:
      // Should be rare because we filter unknown codes before this switch.
#if REMOTE_DEBUG
      Serial.print("IR: Unmapped known code 0x");
      Serial.println(code, HEX);
#endif
      break;
  }

  // 4. Log changes to Serial Monitor for debugging
  if (currentMode != prev) {
    Serial.print(">> Mode Switched: ");
    Serial.println(modeToString(currentMode));
  }

  // 5. Reset receiver to listen again
  IrReceiver.resume();
}