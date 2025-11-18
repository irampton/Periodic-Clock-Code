#include "SettingsCarousel.h"

#include <cassert>
#include <memory>

#include "Display.h"
#include "DS3231_Wrapper.h"
#include "PersistentSettings.h"
#include "clockText.h"

namespace
{
	constexpr int8_t kMinTimezone = -12;
	constexpr int8_t kMaxTimezone = 14;

	ClockText* gClockText = nullptr;
	PersistentSettings* gPersistentSettings = nullptr;
	DS3231_Wrapper* gRtcWrapper = nullptr;
	std::unique_ptr<Carousel> gSettingsCarousel;

	void noOpSetting() {
	}

	void applyTimeMode(TimeMode mode) {
		if (gClockText) {
			gClockText->setTimeMode(mode);
		}
		if (gPersistentSettings) {
			gPersistentSettings->setTimeMode(mode);
		}
	}

	void applyTimeMode12Hour() {
		applyTimeMode(TimeMode::Hour12);
	}

	void applyTimeMode24Hour() {
		applyTimeMode(TimeMode::Hour24);
	}

	size_t timeModeToIndex() {
		if (!gClockText && !gPersistentSettings) {
			return 0;
		}

		const TimeMode mode = gPersistentSettings
			                      ? gPersistentSettings->getTimeMode()
			                      : gClockText->getTimeMode();

		switch (mode) {
			case TimeMode::Hour12:
				return 0;
			case TimeMode::Hour24:
				return 1;
		}
		return 0;
	}

	template <int8_t Offset>
	void applyTimezoneOffset() {
		if (gRtcWrapper) {
			gRtcWrapper->setTimezoneOffset(Offset);
		}
		if (gPersistentSettings) {
			gPersistentSettings->setTimezoneOffsetHours(Offset);
		}
	}

	size_t timezoneToIndex() {
		if (!gPersistentSettings) {
			return static_cast<size_t>(-kMinTimezone);
		}
		int8_t offset = gPersistentSettings->getTimezoneOffsetHours();
		if (offset < kMinTimezone) {
			offset = kMinTimezone;
		} else if (offset > kMaxTimezone) {
			offset = kMaxTimezone;
		}
		return static_cast<size_t>(offset - kMinTimezone);
	}

	void applyDstEnabled() {
		if (gRtcWrapper) {
			gRtcWrapper->setDstEnabled(true);
		}
		if (gPersistentSettings) {
			gPersistentSettings->setDstEnabled(true);
		}
	}

	void applyDstDisabled() {
		if (gRtcWrapper) {
			gRtcWrapper->setDstEnabled(false);
		}
		if (gPersistentSettings) {
			gPersistentSettings->setDstEnabled(false);
		}
	}

	size_t dstIndex() {
		if (!gPersistentSettings) {
			return 0;
		}
		return gPersistentSettings->isDstEnabled() ? 1 : 0;
	}

	CarouselOption settingsOptions[] = {
		{"Settings", noOpSetting},
	};

	CarouselOption timeModeOptions[] = {
		{"12 Hour", applyTimeMode12Hour},
		{"24 Hour", applyTimeMode24Hour},
	};

	CarouselOption timezoneOptions[] = {
		{"-12 UTC", applyTimezoneOffset<-12>},
		{"-11 UTC", applyTimezoneOffset<-11>},
		{"-10 UTC", applyTimezoneOffset<-10>},
		{"-9 UTC", applyTimezoneOffset<-9>},
		{"-8 UTC", applyTimezoneOffset<-8>},
		{"-7 UTC", applyTimezoneOffset<-7>},
		{"-6 UTC", applyTimezoneOffset<-6>},
		{"-5 UTC", applyTimezoneOffset<-5>},
		{"-4 UTC", applyTimezoneOffset<-4>},
		{"-3 UTC", applyTimezoneOffset<-3>},
		{"-2 UTC", applyTimezoneOffset<-2>},
		{"-1 UTC", applyTimezoneOffset<-1>},
		{"+0 UTC", applyTimezoneOffset<0>},
		{"+1 UTC", applyTimezoneOffset<1>},
		{"+2 UTC", applyTimezoneOffset<2>},
		{"+3 UTC", applyTimezoneOffset<3>},
		{"+4 UTC", applyTimezoneOffset<4>},
		{"+5 UTC", applyTimezoneOffset<5>},
		{"+6 UTC", applyTimezoneOffset<6>},
		{"+7 UTC", applyTimezoneOffset<7>},
		{"+8 UTC", applyTimezoneOffset<8>},
		{"+9 UTC", applyTimezoneOffset<9>},
		{"+10 UTC", applyTimezoneOffset<10>},
		{"+11 UTC", applyTimezoneOffset<11>},
		{"+12 UTC", applyTimezoneOffset<12>},
		{"+13 UTC", applyTimezoneOffset<13>},
		{"+14 UTC", applyTimezoneOffset<14>},
	};

	CarouselOption dstOptions[] = {
		{"Off DST", applyDstDisabled},
		{"On DST", applyDstEnabled},
	};

	CarouselItem settingsItems[] = {
		{"Settings", settingsOptions, sizeof(settingsOptions) / sizeof(settingsOptions[0]), 0, nullptr},
		{"Time Mode", timeModeOptions, sizeof(timeModeOptions) / sizeof(timeModeOptions[0]), 0, timeModeToIndex},
		{"Timezone", timezoneOptions, sizeof(timezoneOptions) / sizeof(timezoneOptions[0]), 0, timezoneToIndex},
		{"DST", dstOptions, sizeof(dstOptions) / sizeof(dstOptions[0]), 0, dstIndex},
	};

	constexpr size_t kSettingCount = sizeof(settingsItems) / sizeof(settingsItems[0]);
} // namespace

void initSettingsCarousel(Display* display, ClockText* clockText, PersistentSettings* persistentSettings, DS3231_Wrapper* rtcWrapper) {
	gClockText = clockText;
	gPersistentSettings = persistentSettings;
	gRtcWrapper = rtcWrapper;
	gSettingsCarousel = std::make_unique<Carousel>(settingsItems, kSettingCount, display);
}

Carousel& getSettingsCarousel() {
	assert(gSettingsCarousel && "initSettingsCarousel must be called before accessing the settings carousel");
	return *gSettingsCarousel;
}
