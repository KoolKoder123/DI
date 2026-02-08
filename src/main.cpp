#include "config.h"
#include "leds.h"
#include "beams.h"
#include "remote.h"
#include "mode_intro.h"
#include "mode_round1.h"
#include "mode_round3.h"
#include "mode_round4.h"
#include "patterns.h" // finaleUpdate()

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
// Per-quadrant fixed-lose flicker flags (CODE_R100 -> fixed 40ms)
bool flickerLosePerQuad[NUM_STRIPS_CONNECTED] = {false, false, false, false};
// Per-quadrant steady state
bool steadyActive[NUM_STRIPS_CONNECTED] = {false, false, false, false};
// MODE_R3: track which columns in top-left have been converted to white
bool topLeftColumnsWhite[QUAD_COLS] = {false};
// MODE_R3: track which columns in top-right are currently white (start true)
bool topRightColumnsWhite[QUAD_COLS];
// MODE_R3: per-column color state: 0 = BLUE, 1 = GREEN
uint8_t topLeftColumnColor[QUAD_COLS] = {0};
uint8_t topRightColumnColor[QUAD_COLS] = {0};
// New: steady-armed state for CODE_7 -> CODE_QUEEN_RAP_MORE sequence
bool steadyArmed = false;
// Bottom-left lock: CODE_R2 makes bottom-left stay bright red during MODE_R2
bool bottomLeftLocked = false;

// --- CODE_ROUND_FINAL per-round "lose" actions ---
bool round1BottomLeftEliminated = false;
bool round2LoseSequenceRequested = false;
bool round3ResetAndBlinkRequested = false;
bool round3BlinkActive = false;
bool round4Eliminated[NUM_STRIPS_CONNECTED] = {false, false, false, false};

// Lose-sequence state for CODE_R100 (MODE_R2)
// Sequence: toggle 10 times at exactly 50ms intervals, then draw X over bear.
bool loseSequenceActive[NUM_STRIPS_CONNECTED] = {false, false, false, false};
int loseSequenceCount[NUM_STRIPS_CONNECTED] = {0, 0, 0, 0};
unsigned long loseSequenceNextToggle[NUM_STRIPS_CONNECTED] = {0, 0, 0, 0};

// (Removed per-LED random flashing: top quadrants remain at their default colors)

// --- Random transient flashes (independent of CODE_QUEEN_RAP_MORE/CODE_DOCTOR_RAP_MORE) ---
// These randomly pick LEDs in the top quadrants and flash them a random
// color for a short duration, then restore the original color.
const unsigned long RANDOM_FLASH_TICK_MS = 100; // how often we attempt new flashes
const unsigned long RANDOM_FLASH_DURATION_MS = 300; // flash length
const int RANDOM_FLASH_ATTEMPTS_PER_TICK = 30; // how many random candidates per tick

// Flattened arrays indexed by (q * LEDS_PER_QUAD + physIndex)
bool randomFlashActive[NUM_STRIPS_CONNECTED * LEDS_PER_QUAD] = {false};
uint32_t randomFlashSavedColor[NUM_STRIPS_CONNECTED * LEDS_PER_QUAD] = {0};
unsigned long randomFlashEndTime[NUM_STRIPS_CONNECTED * LEDS_PER_QUAD] = {0};
unsigned long nextRandomFlashTick = 0;

// --- Round 3 blink state (triggered by CODE_ROUND_FINAL) ---
static bool r3BlinkDisplayOn = true;
static unsigned long r3NextBlinkToggle = 0;
static const unsigned long R3_BLINK_INTERVAL_MS = 300;

// Start the Round 2 lose sequence on the bottom-right quadrant.
// This is the same behavior that used to live under CODE_R100.
static void round2StartBottomRightLoseSequence() {
  int idx = Q_BOTTOM_RIGHT;

  // Stop any flicker/steady on other quadrants so the lose sequence is obvious.
  for (int i = 0; i < NUM_STRIPS_CONNECTED; i++) {
    if (i == idx) continue;
    flickerActive[i] = false;
    flickerFastPerQuad[i] = false;
    flickerLosePerQuad[i] = false;
    steadyActive[i] = false;
  }

  // Prepare bottom-right for the lose sequence.
  flickerActive[idx] = false;
  flickerFastPerQuad[idx] = false;
  flickerLosePerQuad[idx] = false;
  steadyActive[idx] = false;

  loseSequenceActive[idx] = true;
  loseSequenceCount[idx] = 0;
  loseSequenceNextToggle[idx] = millis() + 50;
  bearOnPerQuad[idx] = true;

  // Draw bear so the sequence starts from a known state.
  uint32_t bearColor = strips[0].Color(15, 8, 0);
  uint32_t white = strips[0].Color(255, 255, 255);
  drawBearFace(idx, white, bearColor);

  // Clean up a few pixels that occasionally end up white.
  // (This keeps the face consistent before the flashing starts.)
  if (strips[idx].getPixelColor(xyToIndex(6, 11)) == white) strips[idx].setPixelColor(xyToIndex(6, 11), bearColor);
  if (strips[idx].getPixelColor(xyToIndex(10, 11)) == white) strips[idx].setPixelColor(xyToIndex(10, 11), bearColor);
  if (strips[idx].getPixelColor(xyToIndex(7, 10)) == white) strips[idx].setPixelColor(xyToIndex(7, 10), bearColor);
  if (strips[idx].getPixelColor(xyToIndex(9, 10)) == white) strips[idx].setPixelColor(xyToIndex(9, 10), bearColor);
  if (strips[idx].getPixelColor(xyToIndex(7, 7)) == white)  strips[idx].setPixelColor(xyToIndex(7, 7), bearColor);
  if (strips[idx].getPixelColor(xyToIndex(9, 7)) == white)  strips[idx].setPixelColor(xyToIndex(9, 7), bearColor);
  strips[idx].show();

  Serial.println("Round 2: Lose sequence started (bottom-right)");
}

// Enter/reset Round 3 visuals (used on mode entry, and also for CODE_ROUND_FINAL reset).
static void round3EnterState() {
  ledsAllOff();

  // Top-left: blue, Top-right: green
  uint32_t blue = strips[0].Color(0, 0, 255);
  uint32_t green = strips[0].Color(0, 255, 0);
  fillQuad(Q_TOP_LEFT, blue);
  fillQuad(Q_TOP_RIGHT, green);

  // Bottom quadrants: red X
  drawRedX(Q_BOTTOM_LEFT);
  drawRedX(Q_BOTTOM_RIGHT);

  // Ensure steady state on all quadrants so nothing flickers
  for (int i = 0; i < NUM_STRIPS_CONNECTED; i++) {
    flickerActive[i] = false;
    flickerFastPerQuad[i] = false;
    flickerLosePerQuad[i] = false;
    nextToggleTimePerQuad[i] = 0;
    bearOnPerQuad[i] = true;
    steadyActive[i] = true;
  }

  // Initialize column state tracking
  for (int x = 0; x < QUAD_COLS; x++) {
    topLeftColumnsWhite[x] = false;
    topRightColumnsWhite[x] = false;
    topLeftColumnColor[x] = 0;  // blue
    topRightColumnColor[x] = 1; // green
  }

  // Clear any random-flash memory for top quadrants so nothing "restores" later.
  for (int q = Q_TOP_LEFT; q <= Q_TOP_RIGHT; q++) {
    for (uint16_t physIdx = 0; physIdx < LEDS_PER_QUAD; physIdx++) {
      int flat = q * LEDS_PER_QUAD + physIdx;
      randomFlashActive[flat] = false;
      randomFlashSavedColor[flat] = 0;
      randomFlashEndTime[flat] = 0;
    }
  }

  // MODE_R3 does not use bottom-left lock behavior
  bottomLeftLocked = false;

  // Reset blink state
  round3BlinkActive = false;
  round3ResetAndBlinkRequested = false;
  r3BlinkDisplayOn = true;
  r3NextBlinkToggle = 0;
}

// Redraw the top quadrants based on the current per-column color state.
// (Used when blinking turns back "on".)
static void round3RenderTopQuadrantsFromColumns() {
  uint32_t blue = strips[0].Color(0, 0, 255);
  uint32_t green = strips[0].Color(0, 255, 0);

  // Top-left
  for (int x = 0; x < QUAD_COLS; x++) {
    uint32_t col = (topLeftColumnColor[x] == 0) ? blue : green;
    for (int y = 0; y < QUAD_ROWS; y++) {
      strips[Q_TOP_LEFT].setPixelColor(xyToIndex(x, y), col);
    }
  }

  // Top-right
  for (int x = 0; x < QUAD_COLS; x++) {
    uint32_t col = (topRightColumnColor[x] == 0) ? blue : green;
    for (int y = 0; y < QUAD_ROWS; y++) {
      strips[Q_TOP_RIGHT].setPixelColor(xyToIndex(x, y), col);
    }
  }

  strips[Q_TOP_LEFT].show();
  strips[Q_TOP_RIGHT].show();
}

// Pick random candidate LEDs in top quadrants and possibly start flashes
void randomFlashTryStart() {
  if (millis() < nextRandomFlashTick) return;
  nextRandomFlashTick = millis() + RANDOM_FLASH_TICK_MS;

  for (int a = 0; a < RANDOM_FLASH_ATTEMPTS_PER_TICK; a++) {
    // choose top-left or top-right
    int q = (random(0, 2) == 0) ? Q_TOP_LEFT : Q_TOP_RIGHT;
    // choose a usable coordinate inside QUAD_COLS x QUAD_ROWS
    int x = random(0, QUAD_COLS);
    int y = random(0, QUAD_ROWS);
    uint16_t physIdx = xyToIndex(x, y);
    int flat = q * LEDS_PER_QUAD + physIdx;

    if (randomFlashActive[flat]) continue; // already flashing

    // 1/10 chance to start a flash for this candidate
    if (random(8) != 0) continue;

    // Save current color and start flash with a random color
    uint32_t cur = strips[q].getPixelColor(physIdx);
    randomFlashSavedColor[flat] = cur;
    uint8_t r = random(0, 256);
    uint8_t g = random(0, 256);
    uint8_t b = random(0, 256);
    uint32_t newc = strips[q].Color(r, g, b);
    strips[q].setPixelColor(physIdx, newc);
    randomFlashActive[flat] = true;
    randomFlashEndTime[flat] = millis() + RANDOM_FLASH_DURATION_MS;
  }
}

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
      // Intro animations are only allowed to run when the IR receiver is idle.
      runIntroMode();
      break;
      
    case MODE_R1:
      if (currentMode != previousMode && IrReceiver.isIdle()) {
        ledsAllOff();
        roundR1Reset();
        beamsReset();
      }
      if (IrReceiver.isIdle()) roundR1Update(); 
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
          flickerLosePerQuad[i] = false;
        }
        flickerArmed = false; // clear any armed state
        // keep bottomLeftLocked = true so bottom-left stays bright red during MODE_R2
      }

      // CODE_ROUND_FINAL (and legacy CODE_R100) request the lose sequence.
      // We start it only when IR is idle to keep the remote reliable.
      if (round2LoseSequenceRequested && IrReceiver.isIdle()) {
        round2LoseSequenceRequested = false;
        round2StartBottomRightLoseSequence();
      }

      // Per-quadrant flicker handling: allow multiple quadrants to flicker independently
      // Special handling: if a lose-sequence is active, drive its precise timing
      for (int q = 0; q < NUM_STRIPS_CONNECTED; q++) {
        if (loseSequenceActive[q]) {
          unsigned long now = millis();
          if (now >= loseSequenceNextToggle[q]) {
            // Toggle visible state
            bearOnPerQuad[q] = !bearOnPerQuad[q];
            if (bearOnPerQuad[q]) {
              if (!(q == Q_BOTTOM_LEFT && bottomLeftLocked)) {
                uint32_t bearColor = strips[0].Color(15, 8, 0);
                drawBearFace(q, strips[0].Color(255,255,255), bearColor);
              }
            } else {
              if (!(q == Q_BOTTOM_LEFT && bottomLeftLocked)) {
                strips[q].clear();
                strips[q].show();
              }
            }
            loseSequenceCount[q]++;
            // Schedule next toggle exactly 50ms later
            loseSequenceNextToggle[q] = now + 50;

            // If we've toggled 10 times, finish the sequence
            if (loseSequenceCount[q] >= 10) {
              loseSequenceActive[q] = false;
              loseSequenceCount[q] = 0;
              // Ensure final visible state is the bear, then draw X over it
              uint32_t bearColor = strips[0].Color(15, 8, 0);
              drawBearFace(q, strips[0].Color(255,255,255), bearColor);
              // Clear any random flash entries for this quadrant so restores won't override
              for (uint16_t physIdx = 0; physIdx < LEDS_PER_QUAD; physIdx++) {
                int flat = q * LEDS_PER_QUAD + physIdx;
                randomFlashActive[flat] = false;
                randomFlashSavedColor[flat] = strips[q].getPixelColor(physIdx);
                randomFlashEndTime[flat] = 0;
              }
              // Draw the X over the bear (overwrite any overlapping pixels)
              drawRedXOver(q);
              // Stop other flicker flags for this quadrant
              flickerActive[q] = false;
              flickerFastPerQuad[q] = false;
              flickerLosePerQuad[q] = false;
              steadyActive[q] = false;
            }
          }
        }
      }

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
        if (flickerLosePerQuad[q]) {
          // Fixed, deterministic very-fast flicker for CODE_R100
          nextToggleTimePerQuad[q] = millis() + 40;
        } else if (flickerFastPerQuad[q]) {
          nextToggleTimePerQuad[q] = millis() + random(20, 100);
        } else {
          nextToggleTimePerQuad[q] = millis() + random(300, 600);
        }
      }
      // Bear face stays on if not flickering
      break;

    case MODE_R3:
      if (currentMode != previousMode && IrReceiver.isIdle()) {
        round3EnterState();
      }

      // CODE_ROUND_FINAL: reset top quadrants to the Round 3 start state and begin blinking.
      if (round3ResetAndBlinkRequested && IrReceiver.isIdle()) {
        round3EnterState();
        round3ResetAndBlinkRequested = false;
        round3BlinkActive = true;
        r3BlinkDisplayOn = true;
        r3NextBlinkToggle = millis() + R3_BLINK_INTERVAL_MS;
      }

      // Blink mode: toggle top quadrants off/on. (Pause blinking while IR is busy.)
      if (round3BlinkActive) {
        unsigned long now = millis();
        if (r3NextBlinkToggle == 0) {
          r3BlinkDisplayOn = true;
          r3NextBlinkToggle = now + R3_BLINK_INTERVAL_MS;
        }
        if (now >= r3NextBlinkToggle && IrReceiver.isIdle()) {
          r3BlinkDisplayOn = !r3BlinkDisplayOn;
          r3NextBlinkToggle = now + R3_BLINK_INTERVAL_MS;

          if (!r3BlinkDisplayOn) {
            strips[Q_TOP_LEFT].clear();
            strips[Q_TOP_LEFT].show();
            strips[Q_TOP_RIGHT].clear();
            strips[Q_TOP_RIGHT].show();
          } else {
            round3RenderTopQuadrantsFromColumns();
          }
        }
        break;
      }

      // Normal Round 3 visuals: optional random flashes.
      randomFlashTryStart();
      randomFlashUpdate();
      break;

    case MODE_R4:
      // Starts as a copy of MODE_R1 behaviour, but lives in its own file
      // so you can change Round 4 later without touching Round 1.
      if (currentMode != previousMode && IrReceiver.isIdle()) {
        ledsAllOff();
        roundR4Reset();
        beamsReset();
      }
      if (IrReceiver.isIdle()) roundR4Update();
      break;

    case MODE_FINALE: 
      if (IrReceiver.isIdle()) finaleUpdate(); 
      break;
      
    default: 
      // Do nothing in other modes for now
      break;
  }

  // Small pause to keep things stable. When the CODE_R100 fixed very-fast
  // flicker is active we avoid the long 50ms delay so the remote is polled
  // more often (increasing chance of catching button presses between show() calls).
  bool anyLoseActive = false;
  for (int i = 0; i < NUM_STRIPS_CONNECTED; i++) {
    if (flickerLosePerQuad[i]) { anyLoseActive = true; break; }
  }
  if (anyLoseActive) {
    // Poll the remote again quickly to pick up user input during fast flicker
    readRemote();
    delay(1);
  } else {
    delay(10);
  }

  // Save the mode for the next loop to detect transitions
  previousMode = currentMode;
}