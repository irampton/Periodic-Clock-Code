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
		startTimestamp = rtc->getEpoch();
	}
}

void Stopwatch::start() {
	if (!initialized || running) {
		return;
	}
	uint32_t now = rtc->getEpoch();
	startTimestamp = now - elapsedSeconds;
	running = true;
}

void Stopwatch::stop() {
	if (!initialized || !running) {
		return;
	}
	uint32_t now = rtc->getEpoch();
	elapsedSeconds = now - startTimestamp;
	running = false;
}

void Stopwatch::reset() {
	elapsedSeconds = 0;
	if (running) {
		startTimestamp = rtc->getEpoch();
	}
}

void Stopwatch::getTime(uint8_t *hours, uint8_t *minutes, uint8_t *seconds) {
	if (!initialized) {
		if (hours) *hours = 0;
		if (minutes) *minutes = 0;
		if (seconds) *seconds = 0;
		return;
	}
	uint32_t totalSeconds = running ? (rtc->getEpoch() - startTimestamp) : elapsedSeconds;
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

void Stopwatch::toggle() {
	if ( isRunning() ) {
		stop();
	} else {
		start();
	}
}
