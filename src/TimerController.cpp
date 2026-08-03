#include "TimerController.h"

#include <Arduino.h>
#include <algorithm>

#include "Display.h"
#include "OngoingAlarm.h"

void TimerController::init(Display* display, OngoingAlarm* ongoingAlarm) {
	display_ = display;
	ongoingAlarm_ = ongoingAlarm;
	configuredDurationMs_ = 0;
	remainingMs_ = 0;
	startedRemainingMs_ = 0;
	viewDirty_ = true;
}

void TimerController::onEnterMode() { updateBlinkState(); markDirty(); }
void TimerController::onExitMode() {
	editState_ = EditState::None;
	if (display_) display_->blink(false, false, false);
	markDirty();
}

bool TimerController::handleRotaryButtonPress() {
	if (running_ || complete_) return false;
	if (configuredDurationMs_ == 0 && editState_ == EditState::None) {
		setEditing(EditState::Seconds);
		return true;
	}
	switch (editState_) {
		case EditState::None: setEditing(EditState::Seconds); break;
		case EditState::Seconds: setEditing(EditState::Minutes); break;
		case EditState::Minutes: setEditing(EditState::Hours); break;
		case EditState::Hours: setEditing(EditState::None); break;
	}
	return true;
}

bool TimerController::adjustDuration(int delta) {
	if (editState_ == EditState::None || running_ || complete_) return false;
	uint8_t hours = configuredHours();
	uint8_t minutes = configuredMinutes();
	uint8_t seconds = static_cast<uint8_t>((configuredDurationMs_ / 1000u) % 60u);
	if (editState_ == EditState::Seconds) {
		int next = (static_cast<int>(seconds) + delta) % 60;
		if (next < 0) next += 60;
		seconds = static_cast<uint8_t>(next);
	} else if (editState_ == EditState::Minutes) {
		int next = static_cast<int>(minutes) + delta;
		while (next >= 60 && hours < 99) { next -= 60; ++hours; }
		while (next < 0 && hours > 0) { next += 60; --hours; }
		if (next >= 60) next = 59;
		if (next < 0) next = 0;
		minutes = static_cast<uint8_t>(next);
	} else {
		hours = static_cast<uint8_t>(std::clamp(static_cast<int>(hours) + delta, 0, 99));
	}
	configuredDurationMs_ = static_cast<uint32_t>(hours) * 3600000u +
	                         static_cast<uint32_t>(minutes) * 60000u +
	                         static_cast<uint32_t>(seconds) * 1000u;
	remainingMs_ = configuredDurationMs_;
	updateBlinkState();
	markDirty();
	return true;
}

bool TimerController::isEditing() const { return editState_ != EditState::None; }

void TimerController::startOrPause() {
	if (complete_) {
		dismissCompletion();
		reset();
	}
	if (running_) {
		remainingMs_ = currentRemainingMs();
		running_ = false;
	} else if (remainingMs_ > 0 && editState_ == EditState::None) {
		startedAtMs_ = millis();
		startedRemainingMs_ = remainingMs_;
		running_ = true;
	}
	updateBlinkState();
	markDirty();
}

void TimerController::reset() {
	running_ = false;
	complete_ = false;
	remainingMs_ = configuredDurationMs_;
	startedRemainingMs_ = remainingMs_;
	if (ongoingAlarm_) ongoingAlarm_->dismiss(OngoingAlarm::Source::Timer);
	updateBlinkState();
	markDirty();
}

void TimerController::tick() {
	if (!running_) return;
	const uint32_t remaining = currentRemainingMs();
	if (remaining == remainingMs_) return;
	remainingMs_ = remaining;
	markDirty();
	if (remainingMs_ == 0) {
		running_ = false;
		complete_ = true;
		updateBlinkState();
		if (ongoingAlarm_) ongoingAlarm_->activate(OngoingAlarm::Source::Timer);
	}
}

bool TimerController::dismissCompletion() {
	if (!complete_) return false;
	complete_ = false;
	if (ongoingAlarm_) ongoingAlarm_->dismiss(OngoingAlarm::Source::Timer);
	updateBlinkState();
	markDirty();
	return true;
}

bool TimerController::consumeNeedsRedraw() { const bool dirty = viewDirty_; viewDirty_ = false; return dirty; }
bool TimerController::isRunning() const { return running_; }
bool TimerController::isPaused() const { return !running_ && remainingMs_ > 0 && remainingMs_ != configuredDurationMs_ && !complete_; }
bool TimerController::isComplete() const { return complete_; }

uint8_t TimerController::displayHours() const { const uint32_t seconds = (currentRemainingMs() + 999u) / 1000u; return static_cast<uint8_t>(seconds / 3600u); }
uint8_t TimerController::displayMinutes() const { const uint32_t seconds = (currentRemainingMs() + 999u) / 1000u; return static_cast<uint8_t>((seconds / 60u) % 60u); }
uint8_t TimerController::displaySeconds() const { const uint32_t seconds = (currentRemainingMs() + 999u) / 1000u; return static_cast<uint8_t>(seconds % 60u); }
bool TimerController::shouldDisplayHours() const { return displayHours() > 0 || editState_ == EditState::Hours; }

void TimerController::applyStatusColors(CRGB* colors) const {
	if (!colors) return;
	if (complete_) colors[2] = CRGB::Red;
	else if (running_) colors[2] = CRGB::Green;
	else if (isPaused()) colors[2] = CRGB::Orange;
	else colors[2] = CRGB::Blue;
}

void TimerController::setEditing(EditState state) { editState_ = state; updateBlinkState(); markDirty(); }
void TimerController::updateBlinkState() {
	if (!display_) return;
	if (running_) {
		display_->blink(false, true, false);
		return;
	}
	const bool showingHours = configuredHours() > 0 || editState_ == EditState::Hours;
	const bool blinkLeft = editState_ == EditState::Hours || (editState_ == EditState::Minutes && !showingHours);
	const bool blinkRight = editState_ == EditState::Seconds || (editState_ == EditState::Minutes && showingHours);
	display_->blink(blinkLeft, false, blinkRight);
}
void TimerController::markDirty() { viewDirty_ = true; }
uint32_t TimerController::currentRemainingMs() const {
	if (!running_) return remainingMs_;
	const uint32_t elapsed = millis() - startedAtMs_;
	return elapsed >= startedRemainingMs_ ? 0 : startedRemainingMs_ - elapsed;
}
uint8_t TimerController::configuredHours() const { return static_cast<uint8_t>(configuredDurationMs_ / 3600000u); }
uint8_t TimerController::configuredMinutes() const { return static_cast<uint8_t>((configuredDurationMs_ / 60000u) % 60u); }
