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
#define CODE_VOL_MIN 0xF807FF00

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
    case CODE_2:        currentMode = MODE_R2;     break;
    case CODE_3:        currentMode = MODE_R3;     break;
    case CODE_4:        currentMode = MODE_R4;     break;
    case CODE_5:        currentMode = MODE_FINALE; break;
    case CODE_7:
      if (currentMode == MODE_R2) {
        isFlickering = false;
        uint32_t bearColor = strips[0].Color(15, 8, 0);
        for(int q = 0; q < NUM_STRIPS_CONNECTED; q++) {
          drawBearFace(q, strips[0].Color(255,255,255), bearColor);
        }
      }
      break;
    case CODE_8:
      if (currentMode == MODE_R2) {
        isFlickering = true;
        bearOn = true;
        uint32_t bearColor = strips[0].Color(15, 8, 0);
        for(int q = 0; q < NUM_STRIPS_CONNECTED; q++) {
          drawBearFace(q, strips[0].Color(255,255,255), bearColor);
        }
        nextToggleTime = millis() + random(100, 400);
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