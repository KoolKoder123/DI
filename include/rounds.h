#pragma once

// Rounds "hub" include.
//
// This file keeps main.cpp tidy and provides a consistent naming
// convention for each round.
//
// NOTE: Round 2 and Round 3 logic currently live in main.cpp.
// Round 1 and Round 4 are split out because they are planned to diverge.

#include "mode_round1.h"
#include "mode_round4.h"

// -----------------------------
// Backward-compatible wrappers
// -----------------------------
// Older code used round1Update/round1Reset.
// Keep these wrappers so existing files still compile.

static inline void round1Update() { roundR1Update(); }
static inline void round1Reset()  { roundR1Reset(); }

static inline void round4Update() { roundR4Update(); }
static inline void round4Reset()  { roundR4Reset(); }
