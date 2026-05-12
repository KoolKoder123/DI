#pragma once

// Increase tolerance for IR mark/space matching to allow more timing jitter.
// This overrides the library default (25%) if not already defined.
#ifndef TOLERANCE_FOR_DECODERS_MARK_OR_SPACE_MATCHING_PERCENT
#define TOLERANCE_FOR_DECODERS_MARK_OR_SPACE_MATCHING_PERCENT 40
#endif

#include <IRremote.hpp>
#include "config.h"
#include "mode_round2.h"
#include "mode_round1.h"
#include "mode_intro.h"
#include "mode_round4.h"

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
#define CODE_PREVIEW_R1 0xEF1088FF
#define CODE_PREVIEW_R2 0xFB0488FF
#define CODE_PREVIEW_R3 0xC63988FF
#define CODE_PREVIEW_R4 0xFC0388FF

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
    case CODE_PREVIEW_R1:
    case CODE_PREVIEW_R2:
    case CODE_PREVIEW_R3:
    case CODE_PREVIEW_R4:
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
  // If any code other than the preview key is pressed, clear previewFrozen
  if (code != CODE_PREVIEW_R1) previewFrozen = false;
  Mode prev = currentMode;

  // 3. Map buttons to Game Modes
  switch (code) {
    case CODE_INTRO_PATTERN:
      currentMode = MODE_INTRO;
      // Ensure the spiral/intro animation starts from the center every time
      // the intro pattern is selected from the remote.
      introReset();
      break;

    case CODE_PREVIEW_R1: {
      // Show the flower in bottom-left and a jar (2-pixel thick border)
      // in bottom-right quadrant only. Run a single preview animation.
      ledsAllOff();
      drawFlowerInQuad(Q_BOTTOM_LEFT);
      drawRound1JarWithProgress(Q_BOTTOM_RIGHT, 0, strips[0].Color(255,255,255), false);
      animateYellowBallPreviewR1();
      // After a single preview cycle, stop further animations so we do not
      // revert back to the intro spiral. Leave the LEDs showing the final
      // preview state and set a flag so MODE_OFF won't clear the LEDs.
      previewFrozen = true;
      currentMode = MODE_OFF;
      Serial.println("Preview: Flower (Bottom-Left) + Jar (Bottom-Right) + Ball Animation (Stopped)");
      break;
    }

    case CODE_PREVIEW_R2: {
      // Turn entire display yellow
      uint32_t yellow = strips[0].Color(255, 255, 0);
      for (int q = 0; q < NUM_STRIPS_CONNECTED; q++) fillQuad(q, yellow);
      Serial.println("Preview: All Yellow");
      break;
    }

    case CODE_PREVIEW_R3: {
      // Light four specific corner LEDs (one per side) instead of microphones
      ledsAllOff();
      uint32_t white = strips[0].Color(255, 255, 255);
      uint32_t gray = strips[0].Color(25, 25, 25);
      // Bottom-left LED of top-left quadrant -> (2,0) after shift
      strips[Q_TOP_LEFT].setPixelColor(xyToIndex(2, 0), white);
      strips[Q_TOP_LEFT].setPixelColor(xyToIndex(2, 1), white);
      strips[Q_TOP_LEFT].setPixelColor(xyToIndex(2, 2), white);
      strips[Q_TOP_LEFT].setPixelColor(xyToIndex(2, 3), white);
      strips[Q_TOP_LEFT].setPixelColor(xyToIndex(2, 4), white);
      strips[Q_TOP_LEFT].setPixelColor(xyToIndex(2, 5), white);
      strips[Q_TOP_LEFT].setPixelColor(xyToIndex(3, 6), white);
      strips[Q_TOP_LEFT].setPixelColor(xyToIndex(4, 6), white);
      strips[Q_TOP_LEFT].setPixelColor(xyToIndex(5, 6), white);
      strips[Q_TOP_LEFT].setPixelColor(xyToIndex(6, 6), white);
      strips[Q_TOP_LEFT].setPixelColor(xyToIndex(7, 5), white);
      strips[Q_TOP_LEFT].setPixelColor(xyToIndex(7, 4), white);
      strips[Q_TOP_LEFT].setPixelColor(xyToIndex(7, 3), white);
      strips[Q_TOP_LEFT].setPixelColor(xyToIndex(7, 2), white);
      strips[Q_TOP_LEFT].setPixelColor(xyToIndex(7, 1), white);
      strips[Q_TOP_LEFT].setPixelColor(xyToIndex(7, 0), white);

      strips[Q_TOP_LEFT].setPixelColor(xyToIndex(4, 0), white);
      strips[Q_TOP_LEFT].setPixelColor(xyToIndex(5, 0), white);

      strips[Q_TOP_LEFT].setPixelColor(xyToIndex(4, 2), white);
      strips[Q_TOP_LEFT].setPixelColor(xyToIndex(5, 2), white);

      strips[Q_TOP_LEFT].setPixelColor(xyToIndex(4, 4), white);
      strips[Q_TOP_LEFT].setPixelColor(xyToIndex(5, 4), white);

      strips[Q_TOP_LEFT].setPixelColor(xyToIndex(0, 0), white);
      strips[Q_TOP_LEFT].setPixelColor(xyToIndex(0, 1), white);
      strips[Q_TOP_LEFT].setPixelColor(xyToIndex(0, 2), white);
      strips[Q_TOP_LEFT].setPixelColor(xyToIndex(1, 2), white);

      strips[Q_TOP_LEFT].setPixelColor(xyToIndex(9, 0), white);
      strips[Q_TOP_LEFT].setPixelColor(xyToIndex(9, 1), white);
      strips[Q_TOP_LEFT].setPixelColor(xyToIndex(9, 2), white);
      strips[Q_TOP_LEFT].setPixelColor(xyToIndex(8, 2), white);

      //Fill in
      strips[Q_TOP_LEFT].setPixelColor(xyToIndex(3, 0), gray);
      strips[Q_TOP_LEFT].setPixelColor(xyToIndex(3, 1), gray);
      strips[Q_TOP_LEFT].setPixelColor(xyToIndex(3, 2), gray);
      strips[Q_TOP_LEFT].setPixelColor(xyToIndex(3, 3), gray);
      strips[Q_TOP_LEFT].setPixelColor(xyToIndex(3, 4), gray);
      strips[Q_TOP_LEFT].setPixelColor(xyToIndex(3, 5), gray);
      strips[Q_TOP_LEFT].setPixelColor(xyToIndex(4, 5), gray);
      strips[Q_TOP_LEFT].setPixelColor(xyToIndex(5, 5), gray);
      strips[Q_TOP_LEFT].setPixelColor(xyToIndex(6, 5), gray);
      strips[Q_TOP_LEFT].setPixelColor(xyToIndex(6, 4), gray);
      strips[Q_TOP_LEFT].setPixelColor(xyToIndex(6, 3), gray);
      strips[Q_TOP_LEFT].setPixelColor(xyToIndex(6, 2), gray);
      strips[Q_TOP_LEFT].setPixelColor(xyToIndex(6, 1), gray);
      strips[Q_TOP_LEFT].setPixelColor(xyToIndex(6, 0), gray);
      strips[Q_TOP_LEFT].setPixelColor(xyToIndex(5, 1), gray);
      strips[Q_TOP_LEFT].setPixelColor(xyToIndex(4, 1), gray);
      strips[Q_TOP_LEFT].setPixelColor(xyToIndex(4, 3), gray);
      strips[Q_TOP_LEFT].setPixelColor(xyToIndex(5, 3), gray);
      strips[Q_TOP_LEFT].setPixelColor(xyToIndex(4, 5), gray);
      strips[Q_TOP_LEFT].setPixelColor(xyToIndex(5, 5), gray);

      // Top-left LED of bottom-left quadrant -> shifted right by 2
      strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(2, 17), white);
      strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(3, 16), white);
      strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(4, 16), white);
      strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(5, 16), white);
      strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(6, 16), white);
      strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(7, 17), white);

      strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(4, 14), white);
      strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(4, 13), white);
      strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(4, 12), white);
      strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(5, 14), white);
      strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(5, 13), white);
      strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(5, 12), white);
      strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(2, 11), white);
      strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(3, 11), white);
      strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(4, 11), white);
      strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(5, 11), white);
      strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(6, 11), white);
      strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(7, 11), white);
      strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(2, 10), white);
      strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(3, 10), white);
      strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(4, 10), white);
      strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(5, 10), white);
      strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(6, 10), white);
      strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(7, 10), white);

      strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(3, 14), white);
      strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(2, 14), white);
      strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(1, 15), white);
      strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(0, 16), white);
      strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(0, 17), white);

      strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(6, 14), white);
      strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(7, 14), white);
      strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(8, 15), white);
      strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(9, 16), white);
      strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(9, 17), white);
      
      //Fill in
      strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(3, 17), gray);
      strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(4, 17), gray);
      strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(5, 17), gray);
      strips[Q_BOTTOM_LEFT].setPixelColor(xyToIndex(6, 17), gray);

      // Draw remaining microphone parts for top-right and bottom-right
      // Bottom-right LED of top-right quadrant -> shifted right by 10 (2+8)
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(10, 0), white);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(10, 1), white);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(10, 2), white);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(10, 3), white);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(10, 4), white);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(10, 5), white);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(11, 6), white);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(12, 6), white);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(13, 6), white);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(14, 6), white);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(15, 5), white);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(15, 4), white);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(15, 3), white);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(15, 2), white);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(15, 1), white);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(15, 0), white);

      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(12, 0), white);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(13, 0), white);

      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(12, 2), white);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(13, 2), white);

      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(12, 4), white);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(13, 4), white);

      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(8, 0), white);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(8, 1), white);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(8, 2), white);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(9, 2), white);

      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(17, 0), white);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(17, 1), white);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(17, 2), white);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(16, 2), white);

      //Fill in for top-right
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(11, 0), gray);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(11, 1), gray);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(11, 2), gray);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(11, 3), gray);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(11, 4), gray);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(11, 5), gray);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(12, 5), gray);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(13, 5), gray);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(14, 5), gray);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(14, 4), gray);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(14, 3), gray);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(14, 2), gray);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(14, 1), gray);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(14, 0), gray);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(13, 1), gray);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(12, 1), gray);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(12, 3), gray);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(13, 3), gray);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(12, 5), gray);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(13, 5), gray);

      // Top-right LED of bottom-right quadrant -> shifted right by 10 (2+8)
      strips[Q_BOTTOM_RIGHT].setPixelColor(xyToIndex(10, 17), white);
      strips[Q_BOTTOM_RIGHT].setPixelColor(xyToIndex(11, 16), white);
      strips[Q_BOTTOM_RIGHT].setPixelColor(xyToIndex(12, 16), white);
      strips[Q_BOTTOM_RIGHT].setPixelColor(xyToIndex(13, 16), white);
      strips[Q_BOTTOM_RIGHT].setPixelColor(xyToIndex(14, 16), white);
      strips[Q_BOTTOM_RIGHT].setPixelColor(xyToIndex(15, 17), white);

      strips[Q_BOTTOM_RIGHT].setPixelColor(xyToIndex(12, 14), white);
      strips[Q_BOTTOM_RIGHT].setPixelColor(xyToIndex(12, 13), white);
      strips[Q_BOTTOM_RIGHT].setPixelColor(xyToIndex(12, 12), white);
      strips[Q_BOTTOM_RIGHT].setPixelColor(xyToIndex(13, 14), white);
      strips[Q_BOTTOM_RIGHT].setPixelColor(xyToIndex(13, 13), white);
      strips[Q_BOTTOM_RIGHT].setPixelColor(xyToIndex(13, 12), white);
      strips[Q_BOTTOM_RIGHT].setPixelColor(xyToIndex(10, 11), white);
      strips[Q_BOTTOM_RIGHT].setPixelColor(xyToIndex(11, 11), white);
      strips[Q_BOTTOM_RIGHT].setPixelColor(xyToIndex(12, 11), white);
      strips[Q_BOTTOM_RIGHT].setPixelColor(xyToIndex(13, 11), white);
      strips[Q_BOTTOM_RIGHT].setPixelColor(xyToIndex(14, 11), white);
      strips[Q_BOTTOM_RIGHT].setPixelColor(xyToIndex(15, 11), white);
      strips[Q_BOTTOM_RIGHT].setPixelColor(xyToIndex(10, 10), white);
      strips[Q_BOTTOM_RIGHT].setPixelColor(xyToIndex(11, 10), white);
      strips[Q_BOTTOM_RIGHT].setPixelColor(xyToIndex(12, 10), white);
      strips[Q_BOTTOM_RIGHT].setPixelColor(xyToIndex(13, 10), white);
      strips[Q_BOTTOM_RIGHT].setPixelColor(xyToIndex(14, 10), white);
      strips[Q_BOTTOM_RIGHT].setPixelColor(xyToIndex(15, 10), white);

      strips[Q_BOTTOM_RIGHT].setPixelColor(xyToIndex(11, 14), white);
      strips[Q_BOTTOM_RIGHT].setPixelColor(xyToIndex(10, 14), white);
      strips[Q_BOTTOM_RIGHT].setPixelColor(xyToIndex(9, 15), white);
      strips[Q_BOTTOM_RIGHT].setPixelColor(xyToIndex(8, 16), white);
      strips[Q_BOTTOM_RIGHT].setPixelColor(xyToIndex(8, 17), white);

      strips[Q_BOTTOM_RIGHT].setPixelColor(xyToIndex(14, 14), white);
      strips[Q_BOTTOM_RIGHT].setPixelColor(xyToIndex(15, 14), white);
      strips[Q_BOTTOM_RIGHT].setPixelColor(xyToIndex(16, 15), white);
      strips[Q_BOTTOM_RIGHT].setPixelColor(xyToIndex(17, 16), white);
      strips[Q_BOTTOM_RIGHT].setPixelColor(xyToIndex(17, 17), white);
      
      //Fill in for bottom-right
      strips[Q_BOTTOM_RIGHT].setPixelColor(xyToIndex(11, 17), gray);
      strips[Q_BOTTOM_RIGHT].setPixelColor(xyToIndex(12, 17), gray);
      strips[Q_BOTTOM_RIGHT].setPixelColor(xyToIndex(13, 17), gray);
      strips[Q_BOTTOM_RIGHT].setPixelColor(xyToIndex(14, 17), gray);

      // Push the full microphone to display before the note animation
      strips[Q_TOP_LEFT].show();
      strips[Q_BOTTOM_LEFT].show();
      strips[Q_TOP_RIGHT].show();
      strips[Q_BOTTOM_RIGHT].show();

      // Draw two music notes in bottom-left quadrant (do not intersect mic)
      // Colors: vibrant red, blue, purple, orange, yellow, green
      uint32_t noteRed = strips[0].Color(255, 0, 0);
      uint32_t noteBlue = strips[0].Color(0, 0, 255);
      uint32_t notePurple = strips[0].Color(128, 0, 128);
      uint32_t noteOrange = strips[0].Color(255, 165, 0);
      uint32_t noteYellow = strips[0].Color(255, 255, 0);
      uint32_t noteGreen = strips[0].Color(0, 255, 0);
      uint32_t noteColors[6] = {noteRed, noteBlue, notePurple, noteOrange, noteYellow, noteGreen};
      uint8_t pick = random(0, 6);
      uint32_t noteColor = noteColors[pick];

      // animated eighth note: move right 13 total (4 in BL, then 9 in BR)
      const int shapeCount = 10;
      const int sx[shapeCount] = {0, 1, 1, 0, 1, 1, 1, 1, 2, 3};
      const int sy[shapeCount] = {0, 0, 1, 1, 2, 3, 4, 5, 4, 3};

      auto setIfValid = [&](int quad, int lx, int ly, uint32_t col) {
        if (lx < 0 || lx >= QUAD_COLS) return;
        if (ly < 0 || ly >= QUAD_ROWS) return;
        uint16_t id = xyToIndex(lx, ly);
        strips[quad].setPixelColor(id, col);
      };

      auto drawNoteAtGlobal = [&](int gx, int gy, uint32_t col, int prevIdx[], int &prevN) {
        prevN = 0;
        for (int i = 0; i < shapeCount; i++) {
          int ax = gx + sx[i];
          int ay = gy + sy[i];
          int quad = (ax >= QUAD_COLS) ? Q_BOTTOM_RIGHT : Q_BOTTOM_LEFT;
          int lx = (ax >= QUAD_COLS) ? (ax - QUAD_COLS) : ax;
          if (lx >= 0 && lx < QUAD_COLS && ay >= 0 && ay < QUAD_ROWS) {
            setIfValid(quad, lx, ay, col);
            if (prevIdx) {
              prevIdx[prevN++] = (quad << 16) | (lx << 8) | ay;
            }
          }
        }
      };

      auto clearPrev = [&](int prevIdx[], int prevN) {
        for (int i = 0; i < prevN; i++) {
          int packed = prevIdx[i];
          int quad = (packed >> 16) & 0xFF;
          int lx = (packed >> 8) & 0xFF;
          int ly = packed & 0xFF;
          setIfValid(quad, lx, ly, 0);
        }
      };

      // starting origin in bottom-left
      int originX = 10;
      int originY = 11;
      // remember the initial starting position so we can re-use it later
      int initialOriginX = originX;
      int initialOriginY = originY;
      // movement: first 4 steps inside bottom-left, then 9 steps inside bottom-right
      const int stepsLeft = 4;
      const int stepsRight = 9;
      const int totalSteps = stepsLeft + stepsRight;
      int prevStore[64];
      int prevN = 0;

      // draw initial
      drawNoteAtGlobal(originX, originY, noteColor, prevStore, prevN);
      strips[Q_BOTTOM_LEFT].show();
      strips[Q_BOTTOM_RIGHT].show();

      // step loop: each loop moves originX +1 (to the right)
      for (int step = 0; step < totalSteps; step++) {
        delay(10);
        clearPrev(prevStore, prevN);
        originX += 1;
        // draw at new position
        drawNoteAtGlobal(originX, originY, noteColor, prevStore, prevN);
        strips[Q_BOTTOM_LEFT].show();
        strips[Q_BOTTOM_RIGHT].show();
      }

      // After completing the movement, remove the animated eighth-note
      // so it disappears from the display.
      clearPrev(prevStore, prevN);
      strips[Q_BOTTOM_LEFT].show();
      strips[Q_BOTTOM_RIGHT].show();

      // Pause briefly, then draw a double-eighth note at the original
      // starting position in the bottom-left quadrant. The double-eighth
      // uses a newly randomized color from the same palette.
      delay(300);
      uint8_t pick2 = random(0, 6);
      uint32_t doubleNoteColor = noteColors[pick2];

      // Draw a double-eighth: two noteheads (same shape) spaced apart,
      // with a connecting beam near the top of the stems.
      int dPrevStore[64];
      int dPrevN = 0;
      int dOriginX = initialOriginX;

      auto drawDoubleEighthAtGlobal = [&](int gx, int gy, uint32_t col, int store[], int &n) {
        n = 0;
        auto drawAndTrack = [&](int ax, int ay) {
          int quad = (ax >= QUAD_COLS) ? Q_BOTTOM_RIGHT : Q_BOTTOM_LEFT;
          int lx = (ax >= QUAD_COLS) ? (ax - QUAD_COLS) : ax;
          if (lx >= 0 && lx < QUAD_COLS && ay >= 0 && ay < QUAD_ROWS) {
            setIfValid(quad, lx, ay, col);
            if (store) store[n++] = (quad << 16) | (lx << 8) | ay;
          }
        };
        // first notehead: skip i=9 (relative sx=3,sy=3 — the bottom-right pixel)
        for (int i = 0; i < shapeCount; i++) {
          if (i == 9) continue;
          drawAndTrack(gx + sx[i], gy + sy[i]);
        }
        const int secondOffset = 4;
        // second notehead: skip i=8 (sx=2,sy=4) and i=9 (sx=3,sy=3)
        for (int i = 0; i < shapeCount; i++) {
          if (i == 8 || i == 9) continue;
          drawAndTrack(gx + secondOffset + sx[i], gy + sy[i]);
        }
        int beamTopY = gy + 5;
        for (int bx = 1; bx <= (secondOffset + 1); bx++) {
          drawAndTrack(gx + bx, beamTopY);
          drawAndTrack(gx + bx, beamTopY - 1);
        }
      };

      // draw the double-eighth at the original BL start and show
      drawDoubleEighthAtGlobal(dOriginX, initialOriginY, doubleNoteColor, dPrevStore, dPrevN);
      strips[Q_BOTTOM_LEFT].show();
      strips[Q_BOTTOM_RIGHT].show();

      // animate: 2 steps in BL, then 8 steps in BR (10 total, 10 ms each)
      for (int step = 0; step < 10; step++) {
        delay(10);
        clearPrev(dPrevStore, dPrevN);
        dOriginX++;
        drawDoubleEighthAtGlobal(dOriginX, initialOriginY, doubleNoteColor, dPrevStore, dPrevN);
        strips[Q_BOTTOM_LEFT].show();
        strips[Q_BOTTOM_RIGHT].show();
      }

      // clear the double-eighth note after animation completes
      clearPrev(dPrevStore, dPrevN);
      strips[Q_BOTTOM_LEFT].show();
      strips[Q_BOTTOM_RIGHT].show();
      delay(200);

      // Bottom-right LED of top-right quadrant -> shifted right by 10 (2+8)
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(10, 0), white);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(10, 1), white);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(10, 2), white);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(10, 3), white);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(10, 4), white);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(10, 5), white);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(11, 6), white);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(12, 6), white);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(13, 6), white);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(14, 6), white);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(15, 5), white);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(15, 4), white);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(15, 3), white);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(15, 2), white);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(15, 1), white);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(15, 0), white);

      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(12, 0), white);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(13, 0), white);

      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(12, 2), white);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(13, 2), white);

      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(12, 4), white);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(13, 4), white);

      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(8, 0), white);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(8, 1), white);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(8, 2), white);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(9, 2), white);

      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(17, 0), white);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(17, 1), white);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(17, 2), white);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(16, 2), white);

      //Fill in
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(11, 0), gray);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(11, 1), gray);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(11, 2), gray);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(11, 3), gray);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(11, 4), gray);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(11, 5), gray);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(12, 5), gray);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(13, 5), gray);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(14, 5), gray);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(14, 4), gray);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(14, 3), gray);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(14, 2), gray);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(14, 1), gray);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(14, 0), gray);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(13, 1), gray);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(12, 1), gray);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(12, 3), gray);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(13, 3), gray);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(12, 5), gray);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(13, 5), gray);

      // Top-right LED of bottom-right quadrant -> shifted right by 10 (2+8)
      strips[Q_BOTTOM_RIGHT].setPixelColor(xyToIndex(10, 17), white);
      strips[Q_BOTTOM_RIGHT].setPixelColor(xyToIndex(11, 16), white);
      strips[Q_BOTTOM_RIGHT].setPixelColor(xyToIndex(12, 16), white);
      strips[Q_BOTTOM_RIGHT].setPixelColor(xyToIndex(13, 16), white);
      strips[Q_BOTTOM_RIGHT].setPixelColor(xyToIndex(14, 16), white);
      strips[Q_BOTTOM_RIGHT].setPixelColor(xyToIndex(15, 17), white);

      strips[Q_BOTTOM_RIGHT].setPixelColor(xyToIndex(12, 14), white);
      strips[Q_BOTTOM_RIGHT].setPixelColor(xyToIndex(12, 13), white);
      strips[Q_BOTTOM_RIGHT].setPixelColor(xyToIndex(12, 12), white);
      strips[Q_BOTTOM_RIGHT].setPixelColor(xyToIndex(13, 14), white);
      strips[Q_BOTTOM_RIGHT].setPixelColor(xyToIndex(13, 13), white);
      strips[Q_BOTTOM_RIGHT].setPixelColor(xyToIndex(13, 12), white);
      strips[Q_BOTTOM_RIGHT].setPixelColor(xyToIndex(10, 11), white);
      strips[Q_BOTTOM_RIGHT].setPixelColor(xyToIndex(11, 11), white);
      strips[Q_BOTTOM_RIGHT].setPixelColor(xyToIndex(12, 11), white);
      strips[Q_BOTTOM_RIGHT].setPixelColor(xyToIndex(13, 11), white);
      strips[Q_BOTTOM_RIGHT].setPixelColor(xyToIndex(14, 11), white);
      strips[Q_BOTTOM_RIGHT].setPixelColor(xyToIndex(15, 11), white);
      strips[Q_BOTTOM_RIGHT].setPixelColor(xyToIndex(10, 10), white);
      strips[Q_BOTTOM_RIGHT].setPixelColor(xyToIndex(11, 10), white);
      strips[Q_BOTTOM_RIGHT].setPixelColor(xyToIndex(12, 10), white);
      strips[Q_BOTTOM_RIGHT].setPixelColor(xyToIndex(13, 10), white);
      strips[Q_BOTTOM_RIGHT].setPixelColor(xyToIndex(14, 10), white);
      strips[Q_BOTTOM_RIGHT].setPixelColor(xyToIndex(15, 10), white);

      strips[Q_BOTTOM_RIGHT].setPixelColor(xyToIndex(11, 14), white);
      strips[Q_BOTTOM_RIGHT].setPixelColor(xyToIndex(10, 14), white);
      strips[Q_BOTTOM_RIGHT].setPixelColor(xyToIndex(9, 15), white);
      strips[Q_BOTTOM_RIGHT].setPixelColor(xyToIndex(8, 16), white);
      strips[Q_BOTTOM_RIGHT].setPixelColor(xyToIndex(8, 17), white);

      strips[Q_BOTTOM_RIGHT].setPixelColor(xyToIndex(14, 14), white);
      strips[Q_BOTTOM_RIGHT].setPixelColor(xyToIndex(15, 14), white);
      strips[Q_BOTTOM_RIGHT].setPixelColor(xyToIndex(16, 15), white);
      strips[Q_BOTTOM_RIGHT].setPixelColor(xyToIndex(17, 16), white);
      strips[Q_BOTTOM_RIGHT].setPixelColor(xyToIndex(17, 17), white);
      
      //Fill in
      strips[Q_BOTTOM_RIGHT].setPixelColor(xyToIndex(11, 17), gray);
      strips[Q_BOTTOM_RIGHT].setPixelColor(xyToIndex(12, 17), gray);
      strips[Q_BOTTOM_RIGHT].setPixelColor(xyToIndex(13, 17), gray);
      strips[Q_BOTTOM_RIGHT].setPixelColor(xyToIndex(14, 17), gray);

      // Quarter note in Q_TOP_RIGHT near the right-edge microphone.
      // Notehead: 2x2 at (16,8)-(17,9); stem: 5 LEDs up the right side.
      uint32_t quarterNoteColor = noteColors[random(0, 6)];
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(16, 8),  quarterNoteColor);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(17, 8),  quarterNoteColor);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(16, 9),  quarterNoteColor);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(17, 9),  quarterNoteColor);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(17, 7),  quarterNoteColor);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(17, 6),  quarterNoteColor);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(17, 5),  quarterNoteColor);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(17, 4),  quarterNoteColor);
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(17, 3),  quarterNoteColor);

      // Push updates for affected quadrants
      strips[Q_TOP_LEFT].show();
      strips[Q_BOTTOM_LEFT].show();
      strips[Q_TOP_RIGHT].show();
      strips[Q_BOTTOM_RIGHT].show();
      Serial.println("Preview: Four corner LEDs lit");
      break;
    }

    case CODE_PREVIEW_R4: {
      // Turn entire display green
      uint32_t green = strips[0].Color(0, 255, 0);
      for (int q = 0; q < NUM_STRIPS_CONNECTED; q++) fillQuad(q, green);
      Serial.println("Preview: All Green");
      break;
    }
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
      // If compiled with the spiral intro, allow CODE_ROUND_FINAL to
      // immediately show the Round 4 final state when pressed during
      // the INTRO mode (no delay).
      if (currentMode == MODE_INTRO && introIsSpiral()) {
        currentMode = MODE_R4;
        // Mark that this R4 entry is a forced final so main won't reset it.
        round4ForceFinal = true;
        // Mark eliminated for all except bottom-left
        for (int i = 0; i < NUM_STRIPS_CONNECTED; i++) {
          round4Eliminated[i] = (i != Q_BOTTOM_LEFT);
        }
        // Update internal round-4 state and draw final visuals immediately.
        round4ForceFinalDisplay();
        Serial.println("Round 4: Final state shown (from Intro Spiral)");
        break;
      }
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