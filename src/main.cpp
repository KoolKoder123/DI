#include "config.h"
#include "game_state.h"

#include "leds.h"
#include "beams.h"
#include "remote.h"

// Mode handlers
#include "mode_off.h"
#include "mode_intro.h"
#include "mode_round1.h"
#include "mode_round2.h"
#include "mode_round3.h"
#include "mode_round4.h"
#include "mode_finale.h"

// Start the system in OFF mode.
Mode currentMode = MODE_OFF;

// Run the enter() function one time when switching top-level modes.
static Mode lastEnteredTopMode = (Mode)255;

// Print mode changes to the Serial Monitor (debug).
static Mode lastPrintedMode = (Mode)255;

static void enterTopLevelMode(Mode topMode) {
  switch (topMode) {
    case MODE_OFF:    enterOff();    break;
    case MODE_INTRO:  enterIntro();  break;
    case MODE_R1:     enterRound1(); break;
    case MODE_R2:     enterRound2(); break;
    case MODE_R3:     enterRound3(); break;
    case MODE_R4:     enterRound4(); break;
    case MODE_FINALE: enterFinale(); break;
    default: break;
  }
}

static void runTopLevelMode(Mode topMode, bool canShow) {
  switch (topMode) {
    case MODE_OFF:    runOff(canShow);    break;
    case MODE_INTRO:  runIntro(canShow);  break;
    case MODE_R1:     runRound1(canShow); break;
    case MODE_R2:     runRound2(canShow); break;
    case MODE_R3:     runRound3(canShow); break;
    case MODE_R4:     runRound4(canShow); break;
    case MODE_FINALE: runFinale(canShow); break;
    default: break;
  }
}

void setup() {
  Serial.begin(9600);
  while (!Serial) delay(10);

  randomSeed(millis());

  Serial.println("\n--- HIVE MIND SYSTEM START ---");

  // Initialize hardware
  ledsBegin();
  beamsBegin();
  remoteBegin();

  // Start with everything off.
  ledsAllOff();
}

void loop() {
  // 1) Always read the remote first.
  readRemote();

  // 2) Only do LED "show()" work when the IR receiver is idle.
  bool canShow = remoteIsIdle();

  // 3) Print any mode change (including sub-modes).
  if (currentMode != lastPrintedMode) {
    Serial.print(">> Mode: ");
    Serial.println(modeToString(currentMode));
    lastPrintedMode = currentMode;
  }

  // 4) If the top-level mode changed, run the enter() function once.
  Mode top = topLevelMode(currentMode);

  if (top != lastEnteredTopMode) {
    if (!canShow) {
      // Wait until IR is quiet before doing big LED updates.
      delay(1);
      return;
    }
    enterTopLevelMode(top);
    lastEnteredTopMode = top;
  }

  // 5) Run the current mode.
  runTopLevelMode(top, canShow);

  // Small delay to keep the loop stable.
  delay(10);
}
