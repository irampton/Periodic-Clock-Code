#include "SettingsCarousel.h"

#include <cassert>
#include <memory>

#include "Display.h"
#include "clockText.h"

namespace {
ClockText* gClockText = nullptr;
std::unique_ptr<Carousel> gSettingsCarousel;

void noOpSetting() {}

void applyTimeMode12Hour() {
	if (gClockText) {
		gClockText->setTimeMode(TimeMode::Hour12);
	}
}

void applyTimeMode24Hour() {
	if (gClockText) {
		gClockText->setTimeMode(TimeMode::Hour24);
	}
}

size_t timeModeToIndex() {
	if (!gClockText) {
		return 0;
	}

	switch (gClockText->getTimeMode()) {
		case TimeMode::Hour12:
			return 0;
		case TimeMode::Hour24:
			return 1;
	}
	return 0;
}

CarouselOption settingsOptions[] = {
	{"Settings", noOpSetting},
};

CarouselOption timeModeOptions[] = {
	{"12 Hour", applyTimeMode12Hour},
	{"24 Hour", applyTimeMode24Hour},
};

CarouselItem settingsItems[] = {
	{"Settings", settingsOptions, sizeof(settingsOptions) / sizeof(settingsOptions[0]), 0, nullptr},
	{"Time Mode", timeModeOptions, sizeof(timeModeOptions) / sizeof(timeModeOptions[0]), 0, timeModeToIndex},
};

constexpr size_t kSettingCount = sizeof(settingsItems) / sizeof(settingsItems[0]);
} // namespace

void initSettingsCarousel(Display* display, ClockText* clockText) {
	gClockText = clockText;
	gSettingsCarousel = std::make_unique<Carousel>(settingsItems, kSettingCount, display);
}

Carousel& getSettingsCarousel() {
	assert(gSettingsCarousel && "initSettingsCarousel must be called before accessing the settings carousel");
	return *gSettingsCarousel;
}
