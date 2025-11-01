#pragma once
#include <string>
#include <FastLED.h>

#define P_TABLE_SIZE 118

extern const std::string pTable[P_TABLE_SIZE];
extern const uint8_t pTableColor[P_TABLE_SIZE];

inline const std::string& get_element_by_number(int aNumber) {
	if (aNumber < 0 || aNumber >= static_cast<int>(P_TABLE_SIZE)) {
		static const std::string kEmpty = "  ";
		return kEmpty;
	}
	return pTable[aNumber];
}

inline CRGB get_color_by_number(int aNumber) {
	if (aNumber < 0 || aNumber >= static_cast<int>(P_TABLE_SIZE)) {
		return CRGB::Black;
	}

	switch (pTableColor[aNumber]) {
		// 01:   Reactive nonmetals
		case 1:
			return CRGB::Blue2;
		// 02:   Noble gases
		case 2:
			return CRGB::MediumPurple;
		// 03:   Alkali metals
		case 3:
			return CRGB::Red3;
		// 04:   Alkaline earth metals
		case 4:
			return CRGB::Orange2;
		// 05:   Metalloids
		case 5:
			return CRGB::Yellow2;
		// 06:   Post-transition metals
		case 6:
			return CRGB::Green2;
		// 07:   Transition metals
		case 7:
			return CRGB::Cyan2;
		// 08:   Lanthanoids
		case 8:
			return CRGB::Brown;
		// 09:   Actinoids
		case 9:
			return CRGB::Tan;
		// 10:   N/A
		case 10:
			return CRGB::White;
		default:
			return CRGB::White;
	}
}

inline void convert_to_periodic_time(int h, int m, std::string& text, CRGB* colors) {
	std::string hourText = get_element_by_number(h);
	if (hourText.size() == 1) {
		hourText.insert(hourText.begin(), ' ');
	}

	std::string minuteText = get_element_by_number(m);
	if (minuteText.size() == 1) {
		minuteText.push_back(' ');
	}

	text = hourText + ":" + minuteText;
	colors[0] = colors[1] = get_color_by_number(h);
	colors[2] = CRGB::White;
	colors[3] = colors[4] = get_color_by_number(m);
}
