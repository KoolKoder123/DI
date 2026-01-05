#pragma once
#include "config.h"

// Memory to remember if the beam was broken last time we checked
bool beamLast[4] = {false, false, false, false};

void beamsBegin() {
  for (int i = 0; i < 4; i++) {
    pinMode(BEAM_PINS[i], INPUT_PULLUP); // Use internal resistor
  }
  // Initialize beam memory with the actual current state to avoid false positives
  for (int i = 0; i < 4; i++) {
    beamLast[i] = digitalRead(BEAM_PINS[i]) == HIGH;
  }
  Serial.println("Beams: Sensors Active");
}

// Returns TRUE only the exact moment the beam is broken.
// If you hold your hand in the beam, this only returns true once.
bool beamBroken(uint8_t i) {
  // With INPUT_PULLUP: LOW = beam connected (seeing light), HIGH = beam broken
  bool now = digitalRead(BEAM_PINS[i]) == HIGH; // HIGH means beam is broken

  // Logic: It is broken NOW (HIGH), but it wasn't broken BEFORE (was LOW).
  if (now && !beamLast[i]) {
    beamLast[i] = true; // Remember it's broken
    Serial.print("Beam "); Serial.print(i); Serial.println(" broken!");
    return true;
  }

  beamLast[i] = now; // Update memory for next loop
  return false;
}

// Reset the beam memory
void beamsReset() {
  for (int i = 0; i < 4; i++) {
    beamLast[i] = digitalRead(BEAM_PINS[i]) == HIGH;
  }
  Serial.println("Beams: Memory Reset");
}