#include "clockText.h"

namespace {
	enum class ClockColorMode {
		Set,
		Colorful,
	};

	ClockColorMode g_clockColorMode = ClockColorMode::Colorful;
	CRGB g_setModeColor = CRGB::Blue2;

	constexpr uint8_t kDigitSaturation = static_cast<uint8_t>(0.7 * 255);
	constexpr uint8_t kDigitValue = 128;

	CRGB colorForDigit(char ch) {
		uint8_t digit = static_cast<uint8_t>(ch - '0');
		uint8_t hue = static_cast<uint8_t>(digit * 9);
		return CHSV(hue, kDigitSaturation, kDigitValue);
	}

	void applyColors(const std::string& text, CRGB* colors) {
		for (uint8_t i = 0; i < CLOCK_LENGTH; i++) {
			if (i == 2 && text[i] == ':') {
				colors[i] = CRGB::White;
				continue;
			}

			if (g_clockColorMode == ClockColorMode::Colorful && text[i] >= '0' && text[i] <= '9') {
				colors[i] = colorForDigit(text[i]);
			} else {
				colors[i] = g_setModeColor;
			}
		}
	}
}

void setClockTextColorfulMode() {
	g_clockColorMode = ClockColorMode::Colorful;
}

void setClockTextColorMode(CRGB color) {
	g_clockColorMode = ClockColorMode::Set;
	g_setModeColor = color;
}

void prepareTimeString(uint8_t hours, uint8_t minutes, NumberDisplayMode mode, std::string& text, CRGB* colors) {
	switch (mode) {
		case NumberDisplayMode::Hour12: {
			char buffer[6];
			int hour12 = hours % 12;
			if (hour12 == 0) {
				hour12 = 12;
			}
			std::snprintf(buffer, sizeof(buffer), "%02d:%02d", hour12, minutes);
			text.assign(buffer);
			applyColors(text, colors);
			break;
		}
		case NumberDisplayMode::Hour24: {
			char buffer[6];
			std::snprintf(buffer, sizeof(buffer), "%02d:%02d", hours, minutes);
			text.assign(buffer);
			applyColors(text, colors);
			break;
		}
		case NumberDisplayMode::Periodic:
			convert_to_periodic_time(hours, minutes, text, colors);
			break;
	}
}
