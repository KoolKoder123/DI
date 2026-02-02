#pragma once

// Increase tolerance for IR mark/space matching to allow more timing jitter.
// This overrides the library default (25%) if not already defined.
#ifndef TOLERANCE_FOR_DECODERS_MARK_OR_SPACE_MATCHING_PERCENT
#define TOLERANCE_FOR_DECODERS_MARK_OR_SPACE_MATCHING_PERCENT 40
#endif

#include <IRremote.hpp>
#include "config.h"

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
#define CODE_100  0xE619FF00
#define CODE_200 0xF20DFF00
#define CODE_EQ 0xF609FF00

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
    case CODE_CH_MINUS:
    case CODE_CH_PLUS:
    case CODE_0:
    case CODE_1:
    case CODE_2:
    case CODE_3:
    case CODE_4:
    case CODE_5:
    case CODE_7:
    case CODE_8:
    case CODE_9:
    case CODE_PREV:
    case CODE_NEXT:
    case CODE_PAUSE:
    case CODE_100:
    case CODE_200:
    case CODE_EQ:
      return true;
    default:
      return false;
  }
}

// Set to 1 to print extra information about rejected / noisy IR frames.
#ifndef REMOTE_DEBUG
#define REMOTE_DEBUG 0
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
    case CODE_100:
      if (currentMode == MODE_R2) {
        int idx = Q_BOTTOM_RIGHT;
        // Stop any flicker/steady on other quadrants so only bottom-right will run the lose sequence
        for (int i = 0; i < NUM_STRIPS_CONNECTED; i++) {
          if (i == idx) continue;
          flickerActive[i] = false;
          flickerFastPerQuad[i] = false;
          flickerLosePerQuad[i] = false;
        }
        // Start the precise lose-sequence on bottom-right: 10 toggles at 50ms
        loseSequenceActive[idx] = true;
        loseSequenceCount[idx] = 0;
        loseSequenceNextToggle[idx] = millis() + 50; // first toggle after 50ms
        // Ensure quadrant is prepared
        flickerActive[idx] = false;
        flickerFastPerQuad[idx] = false;
        flickerLosePerQuad[idx] = false;
        steadyActive[idx] = false;
        bearOnPerQuad[idx] = true;
        uint32_t bearColor = strips[0].Color(15, 8, 0);
        drawBearFace(idx, strips[0].Color(255,255,255), bearColor);
        // If specific white pixels exist in the bear, change them to brown
        // Coordinates are in quadrant-local (x,y) space.
        uint32_t whiteCol = strips[idx].Color(255,255,255);
        uint32_t brownCol = strips[idx].Color(15,8,0);
        uint16_t p;
        p = xyToIndex(6, 9);
        if (strips[idx].getPixelColor(p) == whiteCol) strips[idx].setPixelColor(p, brownCol);
        p = xyToIndex(11, 13);
        if (strips[idx].getPixelColor(p) == whiteCol) strips[idx].setPixelColor(p, brownCol);
        p = xyToIndex(12, 14);
        if (strips[idx].getPixelColor(p) == whiteCol) strips[idx].setPixelColor(p, brownCol);
        strips[idx].show();
        Serial.println("CODE_100: Bottom-right lose-sequence started (10x @50ms)");
      }
      break;
    // When CODE_8 has armed flicker, these keys choose the quadrant to flicker
    case CODE_NEXT:
      if (currentMode == MODE_R3) {
        // MODE_R3: find the very first GREEN column from the left and
        // convert it to BLUE. Scan the entire top-left (left->right) first,
        // then scan top-right (left->right) only if none found.
        int ql = Q_TOP_LEFT;
        int qr = Q_TOP_RIGHT;
        uint32_t blue = strips[0].Color(0,0,255);
        bool converted = false;
        // Scan top-left fully
        for (int x = 0; x < QUAD_COLS; x++) {
          if (topLeftColumnColor[x] == 1) {
            for (int y = 0; y < QUAD_ROWS; y++) strips[ql].setPixelColor(xyToIndex(x, y), blue);
            topLeftColumnColor[x] = 0; // now blue
            // Clear any random flash state for LEDs in this column so
            // a pending random-flash restore doesn't overwrite the change.
            for (int y = 0; y < QUAD_ROWS; y++) {
              uint16_t physIdx = xyToIndex(x, y);
              int flat = ql * LEDS_PER_QUAD + physIdx;
              randomFlashActive[flat] = false;
              randomFlashSavedColor[flat] = strips[ql].getPixelColor(physIdx);
              randomFlashEndTime[flat] = 0;
            }
            strips[ql].show();
            converted = true;
            break;
          }
        }
        // If nothing converted in top-left, scan top-right
        if (!converted) {
          for (int x = 0; x < QUAD_COLS; x++) {
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
              break;
            }
          }
        }
      } else if (currentMode == MODE_R2) {
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
      if (currentMode == MODE_R3) {
        // MODE_R3: find the first BLUE column from the right and convert it to GREEN.
        // Search top-right first, then top-left. Only convert one column per press.
        int ql = Q_TOP_LEFT;
        int qr = Q_TOP_RIGHT;
        uint32_t green = strips[0].Color(0,255,0);
        bool converted = false;
          for (int x = QUAD_COLS - 1; x >= 0; x--) {
          // Check top-right for BLUE (0)
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
            converted = true;
            break;
          }
        }
        if (!converted) {
          for (int x = QUAD_COLS - 1; x >= 0; x--) {
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
              break;
            }
          }
        }
      } else if (currentMode == MODE_R2) {
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

    case CODE_EQ:
      Serial.println("EQ button pressed");
      break;

    case CODE_200:
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