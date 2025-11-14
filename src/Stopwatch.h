#pragma once

#include <cstdint>

#include "DS3231_Wrapper.h"

class Stopwatch {
public:
	Stopwatch();

	void init(DS3231_Wrapper &rtcSource);
	void start();
	void stop();
	void reset();
	void getTime(uint8_t *hours, uint8_t *minutes, uint8_t *seconds);
	bool isRunning() const;
	void toggle();

private:
	uint32_t currentAbsoluteSeconds();

	DS3231_Wrapper *rtc;
	bool initialized;
	bool running;
	uint32_t startTimestamp;
	uint32_t elapsedSeconds;
	uint32_t dayOffset;
	int lastRawSeconds;
};
