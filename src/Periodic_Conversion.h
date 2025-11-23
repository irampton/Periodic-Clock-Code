#pragma once
#include <string>
#include <FastLED.h>

#define P_TABLE_SIZE 118

extern const std::string pTable[P_TABLE_SIZE];
extern const uint8_t elementFamily[P_TABLE_SIZE];
extern const uint8_t elementColumn[P_TABLE_SIZE];

enum class ColorMode {
	MetalType = 0,
	Column,
	SolidColor,
};

inline ColorMode gColorMode = ColorMode::Column;
inline CRGB gSolidColor = CRGB::Blue;

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

	switch (gColorMode) {
		case ColorMode::MetalType:
			switch (elementFamily[aNumber]) {
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
		case ColorMode::Column:
			switch (elementColumn[aNumber]) {
				// 01:   Hydrogen
				case 1:
					return 0x9340DB;
				// 02:   Alkali metals
				case 2:
					return CRGB::Yellow;
				// 03:   Alkaline earth metals
				case 3:
					return 0x220077;
				// 04:   Boron family
				case 4:
					return 0x00E0E0;
				// 05:   Carbon family
				case 5:
					return 0xA6FFDE;
				// 06:   Nitrogen family
				case 6:
					return 0xFF5511;
				// 07:   Oxygen family
				case 7:
					return 0x1616FF;
				// 08:   Halogens
				case 8:
					return 0x16FF16;
				// 09:   Noble gases
				case 9:
					return 0xFF1616;
				// 10:   Transition metals
				case 10:
					return 0xB0A0E0;
				// 11:   Lanthanoids
				case 11:
					return 0x8B512E;
				// 12:   Actinoids
				case 12:
					return CRGB::Pink;
				// 13:   N/A
				case 13:
					return CRGB::White;
				default:
					return CRGB::White;
			}
		case ColorMode::SolidColor:
			return gSolidColor;
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
