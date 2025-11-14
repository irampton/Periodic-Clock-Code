#include "clockText.h"

void getTime(uint8_t hours, uint8_t minutes, NumberDisplayMode mode, std::string& text, CRGB* colors) {
	switch (mode) {
		case NumberDisplayMode::Hour12: {
			char buffer[6];
			int hour12 = hours % 12;
			if (hour12 == 0) {
				hour12 = 12;
			}
			std::snprintf(buffer, sizeof(buffer), "%2d:%02d", hour12, minutes);
			text.assign(buffer);
			for (uint8_t i = 0; i < CLOCK_LENGTH; i++) {
				colors[i] = CRGB::Blue2;
			}
			break;
		}
		case NumberDisplayMode::Hour24: {
			char buffer[6];
			std::snprintf(buffer, sizeof(buffer), "%02d:%02d", hours, minutes);
			text.assign(buffer);
			for (uint8_t i = 0; i < CLOCK_LENGTH; i++) {
				colors[i] = CRGB::Blue2;
			}
			break;
		}
		case NumberDisplayMode::Periodic:
			convert_to_periodic_time(hours, minutes, text, colors);
			break;
	}
}
