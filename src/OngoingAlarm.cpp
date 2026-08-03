#include "OngoingAlarm.h"

#include <Arduino.h>

#include "Display.h"

namespace {
constexpr uint32_t kBeepPeriodMs = 500;
constexpr uint32_t kBeepOnMs = 180;
constexpr uint16_t kBuzzerFrequencyHz = 2200;
}

void OngoingAlarm::init(Display* display, uint8_t buzzerPin) {
	display_ = display;
	buzzerPin_ = buzzerPin;
	pinMode(buzzerPin_, OUTPUT);
	noTone(buzzerPin_);
}

void OngoingAlarm::activate(Source source) {
	sourceMask_ |= static_cast<uint8_t>(1u << static_cast<uint8_t>(source));
	if (display_) {
		display_->strobe(true);
	}
}

void OngoingAlarm::dismiss(Source source) {
	sourceMask_ &= static_cast<uint8_t>(~(1u << static_cast<uint8_t>(source)));
	if (!isActive()) {
		noTone(buzzerPin_);
		buzzerOn_ = false;
		if (display_) {
			display_->strobe(false);
		}
	}
}

bool OngoingAlarm::isActive(Source source) const {
	return (sourceMask_ & static_cast<uint8_t>(1u << static_cast<uint8_t>(source))) != 0;
}

bool OngoingAlarm::isActive() const {
	return sourceMask_ != 0;
}

void OngoingAlarm::tick() {
	if (!isActive()) {
		return;
	}
	const bool shouldBuzz = (millis() % kBeepPeriodMs) < kBeepOnMs;
	if (shouldBuzz == buzzerOn_) return;
	buzzerOn_ = shouldBuzz;
	if (buzzerOn_) tone(buzzerPin_, kBuzzerFrequencyHz);
	else noTone(buzzerPin_);
}
