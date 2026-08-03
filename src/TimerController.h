#pragma once

#include <cstdint>

#include <FastLED.h>

class Display;
class OngoingAlarm;

class TimerController {
public:
	void init(Display* display, OngoingAlarm* ongoingAlarm);
	void onEnterMode();
	void onExitMode();

	bool handleRotaryButtonPress();
	bool adjustDuration(int delta);
	bool isEditing() const;
	void startOrPause();
	void reset();
	void tick();
	bool dismissCompletion();
	bool consumeNeedsRedraw();

	bool isRunning() const;
	bool isPaused() const;
	bool isComplete() const;
	uint8_t displayHours() const;
	uint8_t displayMinutes() const;
	uint8_t displaySeconds() const;
	bool shouldDisplayHours() const;
	void applyStatusColors(CRGB* colors) const;

private:
	enum class EditState : uint8_t { None, Hours, Minutes, Seconds };
	static constexpr uint32_t kMaxDurationMs = 99u * 3600u * 1000u + 59u * 60u * 1000u + 59u * 1000u;

	void setEditing(EditState state);
	void updateBlinkState();
	void markDirty();
	uint32_t currentRemainingMs() const;
	uint8_t configuredHours() const;
	uint8_t configuredMinutes() const;

	Display* display_ = nullptr;
	OngoingAlarm* ongoingAlarm_ = nullptr;
	uint32_t configuredDurationMs_ = 0;
	uint32_t remainingMs_ = 0;
	uint32_t startedRemainingMs_ = 0;
	uint32_t startedAtMs_ = 0;
	EditState editState_ = EditState::None;
	bool running_ = false;
	bool complete_ = false;
	bool viewDirty_ = true;
};
