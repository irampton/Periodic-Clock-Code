#pragma once

#include <cstdint>
#include <cstdio>
#include <string>

#include <FastLED.h>

#include "Periodic_Conversion.h"

#define CLOCK_LENGTH 5

enum class NumberDisplayMode {
	Numeral,
	Periodic,
};

// Keeps track of whether to render in 12-hour or 24-hour format when displaying numerals.
enum class TimeMode {
	Hour12,
	Hour24,
};

class ClockText {
public:
	explicit ClockText(TimeMode timeMode = TimeMode::Hour24);

	void setTimeMode(TimeMode timeMode);
	TimeMode getTimeMode() const;

	void setColorfulMode();
	void setFixedColor(CRGB color);

	// Prepares a clock string and the color buffer for it. The display mode controls whether
	// the output is numeric or periodic; the stored TimeMode controls 12/24-hour rendering.
	void prepareTimeString(uint8_t hours, uint8_t minutes, NumberDisplayMode displayMode, std::string& text, CRGB* colors) const;
	// Formats a duration pair without applying the clock's 12/24-hour conversion.
	void prepareDurationString(uint8_t high, uint8_t low, NumberDisplayMode displayMode, std::string& text, CRGB* colors) const;

private:
	enum class ClockColorMode {
		Set,
		Colorful,
	};

	static CRGB colorForDigit(char ch);

	void applyColors(const std::string& text, CRGB* colors) const;
	void formatNumeral(uint8_t hours, uint8_t minutes, std::string& text) const;

	TimeMode timeMode_;
	ClockColorMode colorMode_;
	CRGB setModeColor_;
};

