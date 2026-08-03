#include "Stopwatch.h"

#include <Arduino.h>

Stopwatch::Stopwatch()
	: initialized(false), running(false), startMillis(0), elapsedMillis(0) {}

void Stopwatch::init() {
	initialized = true;
	running = false;
	elapsedMillis = 0;
	startMillis = millis();
}

void Stopwatch::start() {
	if (!initialized || running) {
		return;
	}
	startMillis = millis() - elapsedMillis;
	running = true;
}

void Stopwatch::stop() {
	if (!initialized || !running) {
		return;
	}
	elapsedMillis = millis() - startMillis;
	running = false;
}

void Stopwatch::reset() {
	elapsedMillis = 0;
	if (running) {
		startMillis = millis();
	}
}

void Stopwatch::getTime(uint8_t *hours, uint8_t *minutes, uint8_t *seconds) {
	if (!initialized) {
		if (hours) *hours = 0;
		if (minutes) *minutes = 0;
		if (seconds) *seconds = 0;
		return;
	}
	const uint32_t elapsed = running ? (millis() - startMillis) : elapsedMillis;
	const uint32_t totalSeconds = elapsed / 1000u;
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
