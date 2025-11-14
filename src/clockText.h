#pragma once

#include <cstdint>
#include <cstdio>
#include <string>
#include <FastLED.h>
#include "Periodic_Conversion.h"

#define CLOCK_LENGTH 5

enum class NumberDisplayMode {
	Hour12,
	Hour24,
	Periodic,
};

void getTime(uint8_t hours, uint8_t minutes, NumberDisplayMode mode, std::string& text, CRGB* colors);
