#include "clockText.h"

namespace
{
	constexpr uint8_t kDigitSaturation = static_cast<uint8_t>(0.9 * 255);
	constexpr uint8_t kDigitValue = 196;
}

ClockText::ClockText(TimeMode timeMode)
	: timeMode_(timeMode), colorMode_(ClockColorMode::Colorful), setModeColor_(CRGB::Blue2) {
}

void ClockText::setTimeMode(TimeMode timeMode) {
	timeMode_ = timeMode;
}

TimeMode ClockText::getTimeMode() const {
	return timeMode_;
}

void ClockText::setColorfulMode() {
	colorMode_ = ClockColorMode::Colorful;
}

void ClockText::setFixedColor(CRGB color) {
	colorMode_ = ClockColorMode::Set;
	setModeColor_ = color;
}

CRGB ClockText::colorForDigit(char ch) {
	uint8_t digit = static_cast<uint8_t>(ch - '0');
	uint8_t hue = static_cast<uint8_t>(digit * 9);
	return CHSV(hue, kDigitSaturation, kDigitValue);
}

void ClockText::applyColors(const std::string& text, CRGB* colors) const {
	for (uint8_t i = 0; i < CLOCK_LENGTH; i++) {
		if (i == 2 && text[i] == ':') {
			colors[i] = CRGB::White;
			continue;
		}

		if (colorMode_ == ClockColorMode::Colorful && text[i] >= '0' && text[i] <= '9') {
			colors[i] = colorForDigit(text[i]);
		} else {
			colors[i] = setModeColor_;
		}
	}
}

void ClockText::formatNumeral(uint8_t hours, uint8_t minutes, std::string& text) const {
	char buffer[6];
	uint8_t renderedHour = hours;

	if (timeMode_ == TimeMode::Hour12) {
		renderedHour = static_cast<uint8_t>(hours % 12);
		if (renderedHour == 0) {
			renderedHour = 12;
		}
		std::snprintf(buffer, sizeof(buffer), "%2d:%02d", renderedHour, minutes);
	} else {
		std::snprintf(buffer, sizeof(buffer), "%02d:%02d", hours, minutes);
	}

	text.assign(buffer);
}

void ClockText::prepareTimeString(uint8_t hours, uint8_t minutes, NumberDisplayMode displayMode, std::string& text, CRGB* colors) const {
	switch (displayMode) {
		case NumberDisplayMode::Numeral:
			formatNumeral(hours, minutes, text);
			applyColors(text, colors);
			break;
		case NumberDisplayMode::Periodic:
			uint8_t correctedHours = timeMode_ == TimeMode::Hour12
				                         ? hours % 12 == 0
					                           ? 12
					                           : hours % 12
				                         : hours;
			convert_to_periodic_time(correctedHours, minutes, text, colors);
			break;
	}
}

void ClockText::prepareDurationString(uint8_t high, uint8_t low, NumberDisplayMode displayMode, std::string& text, CRGB* colors) const {
	switch (displayMode) {
		case NumberDisplayMode::Numeral: {
			char buffer[6];
			std::snprintf(buffer, sizeof(buffer), "%02u:%02u", high, low);
			text.assign(buffer);
			applyColors(text, colors);
			break;
		}
		case NumberDisplayMode::Periodic:
			convert_to_periodic_time(high, low, text, colors);
			break;
	}
}
