#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include <FastLED.h>

#include "DS3231_Wrapper.h"
#include "clockText.h"
#include "PersistentSettings.h"

class Display;
class OngoingAlarm;

class AlarmController {
public:
	static constexpr size_t kAlarmCount = 8;

	void init(DS3231_Wrapper* rtc, PersistentSettings* settings, OngoingAlarm* ongoingAlarm);
	void setDisplay(Display* display);
	void onEnterMode();
	void onExitMode();

	bool selectNextAlarm();
	bool toggleCurrentAlarmActive();
	bool handleRotaryButtonPress();
	bool adjustCurrentAlarm(int delta);
	bool isEditing() const;

	uint8_t currentHours() const;
	uint8_t currentMinutes() const;
	size_t currentIndex() const;
	bool isCurrentAlarmEnabled() const;
	bool isAlarmRinging() const;

	void applyStatusColors(CRGB* colors) const;

	void tick();
	bool dismissActiveAlarm();

	bool consumeNeedsRedraw();

private:
	enum class EditState : uint8_t {
		None,
		Hours,
		Minutes,
	};

	struct AlarmEntry {
		uint8_t hours = 6;
		uint8_t minutes = 0;
		bool enabled = false;
		bool modified = false;
		bool triggeredThisMinute = false;
		uint16_t guardMinuteOfDay = 0;
	};

	AlarmEntry& currentAlarm();
	const AlarmEntry& currentAlarm() const;

	void markDirty();
	void stopEditing();
	void startAlarm(size_t index, uint16_t currentMinuteOfDay);
	void activateAlarm();
	void updateBlinkState();
	static uint16_t minuteOfDay(uint8_t hour, uint8_t minute);
	static uint16_t minuteOfDay(const DS3231_Wrapper::HoursMinutes& time);
	void refreshGuardForCurrentTime(uint16_t currentMinuteOfDay);
	void clearGuard(AlarmEntry& alarm);
	void markModified(AlarmEntry& alarm);
	size_t highestSelectableIndex() const;
	void loadFromStorage();
	void saveToStorage();

	std::array<AlarmEntry, kAlarmCount> alarms_{};
	size_t currentIndex_ = 0;
	EditState editState_ = EditState::None;
	DS3231_Wrapper* rtc_ = nullptr;
	PersistentSettings* settings_ = nullptr;
	Display* display_ = nullptr;
	OngoingAlarm* ongoingAlarm_ = nullptr;
	bool viewDirty_ = false;
	bool alarmRinging_ = false;
	int8_t ringingAlarmIndex_ = -1;
	uint16_t ringingStartMinuteOfDay_ = 0;
	bool alarmsDirty_ = false;
};
