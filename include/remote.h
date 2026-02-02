#pragma once

// Increase tolerance for IR mark/space matching to allow more timing jitter.
#ifndef TOLERANCE_FOR_DECODERS_MARK_OR_SPACE_MATCHING_PERCENT
#define TOLERANCE_FOR_DECODERS_MARK_OR_SPACE_MATCHING_PERCENT 40
#endif

#include <IRremote.hpp>
#include "config.h"
#include "game_state.h"

// -----------------------------
// Remote button codes (NEC)
// -----------------------------
#define CODE_CH_MINUS  0xBA45FF00
#define CODE_CH_PLUS   0xB847FF00
#define CODE_0         0xE916FF00
#define CODE_1         0xF30CFF00
#define CODE_2         0xE718FF00
#define CODE_3         0xA15EFF00
#define CODE_4         0xF708FF00
#define CODE_5         0xE31CFF00
#define CODE_7         0xBD42FF00
#define CODE_8         0xAD52FF00
#define CODE_9         0xB54AFF00
#define CODE_PREV      0xBB44FF00
#define CODE_NEXT      0xBF40FF00
#define CODE_PAUSE     0xBC43FF00
#define CODE_LOSE      0xE619FF00
#define CODE_WIN       0xF20DFF00
#define CODE_EQ        0xF609FF00

// Turn on to print raw codes to Serial.
#define REMOTE_DEBUG 0

static inline void remoteBegin() {
  IrReceiver.begin(IR_RECEIVER_PIN, ENABLE_LED_FEEDBACK);
  Serial.println("IR: Receiver ready");
}

static inline bool remoteIsIdle() {
  return IrReceiver.isIdle();
}

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
    case CODE_LOSE:
    case CODE_WIN:
    case CODE_EQ:
      return true;
    default:
      return false;
  }
}

static inline void setModeIfDifferent(Mode m) {
  if (currentMode == m) return;
  currentMode = m;
}

static inline void readRemote() {
  if (!IrReceiver.decode()) return;

  uint16_t flags = IrReceiver.decodedIRData.flags;

  // If the library reports a repeat frame, ignore it.
  // (We want one action per press, not an auto-repeat.)
#ifdef IRDATA_FLAGS_IS_REPEAT
  if (flags & IRDATA_FLAGS_IS_REPEAT) {
    IrReceiver.resume();
    return;
  }
#endif

  // Ignore overflow / parity failures when those flags exist.
#ifdef IRDATA_FLAGS_WAS_OVERFLOW
  if (flags & IRDATA_FLAGS_WAS_OVERFLOW) {
    IrReceiver.resume();
    return;
  }
#endif

#ifdef IRDATA_FLAGS_PARITY_FAILED
  if (flags & IRDATA_FLAGS_PARITY_FAILED) {
    IrReceiver.resume();
    return;
  }
#endif

  uint32_t code = IrReceiver.decodedIRData.decodedRawData;

  // Ignore unknown / partial codes.
  if (!isKnownRemoteCode(code)) {
#if REMOTE_DEBUG
    Serial.print("IR: Unknown code 0x");
    Serial.println(code, HEX);
#endif
    IrReceiver.resume();
    return;
  }

  // Simple debounce: ignore the same code if it happens too soon.
  static uint32_t lastCode = 0;
  static unsigned long lastCodeTime = 0;
  unsigned long now = millis();
  if (code == lastCode && (now - lastCodeTime) < 120) {
    IrReceiver.resume();
    return;
  }
  lastCode = code;
  lastCodeTime = now;

#if REMOTE_DEBUG
  Serial.print("IR: 0x");
  Serial.println(code, HEX);
#endif

  // Map button presses to Modes.
  switch (code) {
    case CODE_CH_MINUS:
      setModeIfDifferent(MODE_INTRO);
      break;

    case CODE_CH_PLUS:
      setModeIfDifferent(MODE_FINALE);
      break;

    case CODE_0:
      setModeIfDifferent(MODE_OFF);
      break;

    case CODE_1:
      setModeIfDifferent(MODE_R1);
      break;

    case CODE_2:
      // If already in Round 2, CODE_2 is a one-shot "lock bottom-left" command.
      if (isRound2Mode(currentMode)) {
        setModeIfDifferent(MODE_R2_LOCK_BOTTOM_LEFT);
      } else {
        setModeIfDifferent(MODE_R2);
      }
      break;

    case CODE_3:
      setModeIfDifferent(MODE_R3);
      break;

    case CODE_4:
      setModeIfDifferent(MODE_R4);
      break;

    case CODE_5:
      setModeIfDifferent(MODE_FINALE);
      break;

    case CODE_7:
      if (isRound2Mode(currentMode)) setModeIfDifferent(MODE_R2_STEADY_ARMED);
      break;

    case CODE_8:
      if (isRound2Mode(currentMode)) setModeIfDifferent(MODE_R2_FLICKER_ARMED);
      break;

    case CODE_9:
      if (isRound2Mode(currentMode)) setModeIfDifferent(MODE_R2_FLICKER_FAST_ARMED);
      break;

    case CODE_LOSE:
      if (isRound2Mode(currentMode)) setModeIfDifferent(MODE_R2_TRIGGER_LOSE_SEQUENCE);
      break;

    case CODE_NEXT:
      // Round 3: move one green column to blue.
      if (isRound3Mode(currentMode)) {
        setModeIfDifferent(MODE_R3_STEP_GREEN_TO_BLUE);
        break;
      }

      // Round 2: top-right selector (depends on armed state).
      if (currentMode == MODE_R2_STEADY_ARMED) setModeIfDifferent(MODE_R2_STEADY_Q_TOP_RIGHT);
      if (currentMode == MODE_R2_FLICKER_ARMED) setModeIfDifferent(MODE_R2_FLICKER_Q_TOP_RIGHT);
      if (currentMode == MODE_R2_FLICKER_FAST_ARMED) setModeIfDifferent(MODE_R2_FLICKER_FAST_Q_TOP_RIGHT);
      break;

    case CODE_PREV:
      // Round 3: move one blue column to green.
      if (isRound3Mode(currentMode)) {
        setModeIfDifferent(MODE_R3_STEP_BLUE_TO_GREEN);
        break;
      }

      // Round 2: top-left selector (depends on armed state).
      if (currentMode == MODE_R2_STEADY_ARMED) setModeIfDifferent(MODE_R2_STEADY_Q_TOP_LEFT);
      if (currentMode == MODE_R2_FLICKER_ARMED) setModeIfDifferent(MODE_R2_FLICKER_Q_TOP_LEFT);
      if (currentMode == MODE_R2_FLICKER_FAST_ARMED) setModeIfDifferent(MODE_R2_FLICKER_FAST_Q_TOP_LEFT);
      break;

    case CODE_PAUSE:
      // Round 2: bottom-right selector (depends on armed state).
      if (currentMode == MODE_R2_STEADY_ARMED) setModeIfDifferent(MODE_R2_STEADY_Q_BOTTOM_RIGHT);
      if (currentMode == MODE_R2_FLICKER_ARMED) setModeIfDifferent(MODE_R2_FLICKER_Q_BOTTOM_RIGHT);
      if (currentMode == MODE_R2_FLICKER_FAST_ARMED) setModeIfDifferent(MODE_R2_FLICKER_FAST_Q_BOTTOM_RIGHT);
      break;

    default:
      // Other buttons are currently unused.
      break;
  }

  IrReceiver.resume();
}
