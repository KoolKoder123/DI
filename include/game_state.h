#pragma once
#include "config.h"

// currentMode is the one "big switch" that drives the whole project.
// The IR remote changes it, and main.cpp runs logic based on it.
extern Mode currentMode;
