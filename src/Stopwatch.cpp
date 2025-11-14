#include "Stopwatch.h"

namespace {
	constexpr uint32_t SECONDS_PER_DAY = 24u * 60u * 60u;
}

Stopwatch::Stopwatch()
	: rtc(nullptr), initialized(false), running(false), startTimestamp(0), elapsedSeconds(0), dayOffset(0), lastRawSeconds(-1) {}

void Stopwatch::init(DS3231_Wrapper &rtcSource) {
	rtc = &rtcSource;
	initialized = rtc != nullptr;
	running = false;
	elapsedSeconds = 0;
	dayOffset = 0;
	lastRawSeconds = -1;
	if (initialized) {
		startTimestamp = currentAbsoluteSeconds();
	}
}

void Stopwatch::start() {
	if (!initialized || running) {
		return;
	}
	uint32_t now = currentAbsoluteSeconds();
	startTimestamp = now - elapsedSeconds;
	running = true;
}

void Stopwatch::stop() {
	if (!initialized || !running) {
		return;
	}
	uint32_t now = currentAbsoluteSeconds();
	elapsedSeconds = now - startTimestamp;
	running = false;
}

void Stopwatch::reset() {
	elapsedSeconds = 0;
	if (running) {
		startTimestamp = currentAbsoluteSeconds();
	}
}

void Stopwatch::getTime(uint8_t *hours, uint8_t *minutes, uint8_t *seconds) {
	if (!initialized) {
		if (hours) *hours = 0;
		if (minutes) *minutes = 0;
		if (seconds) *seconds = 0;
		return;
	}
	uint32_t totalSeconds = running ? (currentAbsoluteSeconds() - startTimestamp) : elapsedSeconds;
	uint32_t totalHours = totalSeconds / 3600u;
	uint8_t hh = static_cast<uint8_t>(totalHours > 255u ? 255u : totalHours);
	uint8_t mm = static_cast<uint8_t>((totalSeconds / 60u) % 60u);
	uint8_t ss = static_cast<uint8_t>(totalSeconds % 60u);
	if (hours) *hours = hh;
	if (minutes) *minutes = mm;
	if (seconds) *seconds = ss;
}

bool Stopwatch::isRunning() const {
	return running;
}

uint32_t Stopwatch::currentAbsoluteSeconds() {
	if (!rtc) {
		return 0;
	}
	uint32_t hours = static_cast<uint32_t>(rtc->getHours());
	uint32_t minutes = static_cast<uint32_t>(rtc->getMinutes());
	uint32_t seconds = static_cast<uint32_t>(rtc->getSeconds());
	int rawSeconds = static_cast<int>(hours * 3600u + minutes * 60u + seconds);
	if (lastRawSeconds == -1) {
		lastRawSeconds = rawSeconds;
		return dayOffset + static_cast<uint32_t>(rawSeconds);
	}
	if (rawSeconds < lastRawSeconds) {
		dayOffset += SECONDS_PER_DAY;
	}
	lastRawSeconds = rawSeconds;
	return dayOffset + static_cast<uint32_t>(rawSeconds);
}

void Stopwatch::toggle() {
	if ( isRunning() ) {
		stop();
	} else {
		start();
	}
}
