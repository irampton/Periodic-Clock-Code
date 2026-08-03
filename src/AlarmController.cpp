#include "AlarmController.h"

#include <algorithm>
#include <array>

#include "Display.h"
#include "OngoingAlarm.h"

namespace {
constexpr int kHoursPerDay = 24;
constexpr int kMinutesPerHour = 60;
constexpr int kMinutesPerDay = kHoursPerDay * kMinutesPerHour;
constexpr int kAutoClearMinutes = 1;
}

void AlarmController::init(DS3231_Wrapper* rtc, PersistentSettings* settings, OngoingAlarm* ongoingAlarm) {
	rtc_ = rtc;
	settings_ = settings;
	ongoingAlarm_ = ongoingAlarm;
	loadFromStorage();
	viewDirty_ = true;
}

void AlarmController::setDisplay(Display* display) {
	display_ = display;
	if (display_) {
		updateBlinkState();
		display_->strobe(ongoingAlarm_ && ongoingAlarm_->isActive());
	}
}

void AlarmController::onEnterMode() {
    markDirty();
}

void AlarmController::onExitMode() {
	saveToStorage();
	stopEditing();
}

bool AlarmController::selectNextAlarm() {
    if (alarms_.empty()) {
        return false;
    }

    const size_t selectableLimit = highestSelectableIndex();
    if (currentIndex_ > selectableLimit) {
        currentIndex_ = selectableLimit;
        stopEditing();
        markDirty();
        return true;
    }

    if (selectableLimit == 0 && currentIndex_ == 0) {
        return false;
    }

    size_t nextIndex = (currentIndex_ >= selectableLimit) ? 0 : currentIndex_ + 1u;
    if (nextIndex == currentIndex_) {
        return false;
    }

    currentIndex_ = nextIndex;
    stopEditing();
    markDirty();
    return true;
}

bool AlarmController::toggleCurrentAlarmActive() {
	auto& alarm = currentAlarm();
	alarm.enabled = !alarm.enabled;
	if (alarm.enabled) {
		stopEditing();
	}
	markModified(alarm);
	clearGuard(alarm);
	markDirty();
	return alarm.enabled;
}

bool AlarmController::handleRotaryButtonPress() {
    auto& alarm = currentAlarm();
    if (alarm.enabled && editState_ == EditState::None) {
        return false;
    }

    switch (editState_) {
        case EditState::None:
            editState_ = EditState::Hours;
            break;
        case EditState::Hours:
            editState_ = EditState::Minutes;
            break;
        case EditState::Minutes:
            editState_ = EditState::None;
            break;
    }
	updateBlinkState();
    markDirty();
    return true;
}

bool AlarmController::adjustCurrentAlarm(int delta) {
    if (editState_ == EditState::None) {
        return false;
    }

    auto& alarm = currentAlarm();
    bool updated = false;
    if (editState_ == EditState::Hours) {
        int value = static_cast<int>(alarm.hours) + delta;
        value %= kHoursPerDay;
        if (value < 0) {
            value += kHoursPerDay;
        }
        uint8_t newValue = static_cast<uint8_t>(value);
        if (alarm.hours != newValue) {
            alarm.hours = newValue;
            updated = true;
        }
    } else if (editState_ == EditState::Minutes) {
        int value = static_cast<int>(alarm.minutes) + delta;
        value %= kMinutesPerHour;
        if (value < 0) {
            value += kMinutesPerHour;
        }
        uint8_t newValue = static_cast<uint8_t>(value);
        if (alarm.minutes != newValue) {
            alarm.minutes = newValue;
            updated = true;
        }
    }

	if (updated) {
		markModified(alarm);
		clearGuard(alarm);
	}
	updateBlinkState();
	markDirty();
	return true;
}

bool AlarmController::isEditing() const {
    return editState_ != EditState::None;
}

uint8_t AlarmController::currentHours() const {
    return currentAlarm().hours;
}

uint8_t AlarmController::currentMinutes() const {
    return currentAlarm().minutes;
}

size_t AlarmController::currentIndex() const {
    return currentIndex_;
}

bool AlarmController::isCurrentAlarmEnabled() const {
    return currentAlarm().enabled;
}

bool AlarmController::isAlarmRinging() const {
    return alarmRinging_;
}

void AlarmController::applyStatusColors(CRGB* colors) const {
	if (!colors) {
		return;
	}

	auto softScale = [](const CRGB& color) -> CRGB {
		const float factor = 0.7f;
		const auto scaleComp = [factor](uint8_t component) -> uint8_t
		{
			return static_cast<uint8_t>(std::clamp(static_cast<int>(component * factor), 0, 255));
		};
		return CRGB(scaleComp(color.r), scaleComp(color.g), scaleComp(color.b));
	};

	CRGB baseColor = softScale(CRGB::Blue);
	if (alarmRinging_) {
		baseColor = CRGB::Red;
	} else if (currentAlarm().enabled) {
		baseColor = softScale(CRGB::Green);
	} else {
		baseColor = softScale(CRGB::Blue);
	}

	for (uint8_t i = 0; i < CLOCK_LENGTH; ++i) {
		if (i == 2) {
			colors[i] = alarmRinging_ ? CRGB::White : CRGB::Gray;
			continue;
		}
		colors[i] = baseColor;
	}

	if (alarmRinging_) {
		return;
	}
}

void AlarmController::tick() {
	if (!rtc_) {
		return;
	}

	const auto currentTime = rtc_->getCachedHoursMinutes();
	const uint16_t currentMinuteOfDay = minuteOfDay(currentTime);
	refreshGuardForCurrentTime(currentMinuteOfDay);

	if (!alarmRinging_) {
		for (size_t i = 0; i < alarms_.size(); ++i) {
			auto& alarm = alarms_[i];
			if (!alarm.enabled || alarm.triggeredThisMinute) {
				continue;
			}
			if (alarm.hours != currentTime.hour || alarm.minutes != currentTime.minute) {
				continue;
			}
			alarm.triggeredThisMinute = true;
			alarm.guardMinuteOfDay = currentMinuteOfDay;
			startAlarm(i, currentMinuteOfDay);
			break;
		}
	} else {
		const int diffMinutes = (static_cast<int>(currentMinuteOfDay) - static_cast<int>(ringingStartMinuteOfDay_) + kMinutesPerDay) % kMinutesPerDay;
		if (diffMinutes >= kAutoClearMinutes) {
			dismissActiveAlarm();
		}
	}

	if (alarmRinging_) {
		activateAlarm();
	}
}

bool AlarmController::dismissActiveAlarm() {
	if (!alarmRinging_) {
		return false;
	}
	alarmRinging_ = false;
	ringingAlarmIndex_ = -1;
	ringingStartMinuteOfDay_ = 0;
	if (ongoingAlarm_) ongoingAlarm_->dismiss(OngoingAlarm::Source::Alarm);
	markDirty();
	return true;
}

bool AlarmController::consumeNeedsRedraw() {
    if (!viewDirty_) {
        return false;
    }
    viewDirty_ = false;
    return true;
}

AlarmController::AlarmEntry& AlarmController::currentAlarm() {
    return alarms_[currentIndex_];
}

const AlarmController::AlarmEntry& AlarmController::currentAlarm() const {
    return alarms_[currentIndex_];
}

void AlarmController::markDirty() {
    viewDirty_ = true;
}

void AlarmController::stopEditing() {
    if (editState_ != EditState::None) {
        editState_ = EditState::None;
		updateBlinkState();
    }
}

void AlarmController::startAlarm(size_t index, uint16_t currentMinuteOfDay) {
	alarmRinging_ = true;
	ringingAlarmIndex_ = static_cast<int8_t>(index);
	ringingStartMinuteOfDay_ = currentMinuteOfDay;
	stopEditing();
	markDirty();
	if (ongoingAlarm_) ongoingAlarm_->activate(OngoingAlarm::Source::Alarm);
	activateAlarm();
}

void AlarmController::activateAlarm() {
	// Placeholder for buzzer/LED activation logic.
}

void AlarmController::updateBlinkState() {
	if (!display_) {
		return;
	}
	const bool blinkHours = (editState_ == EditState::Hours);
	const bool blinkMinutes = (editState_ == EditState::Minutes);
	display_->blink(blinkHours, false, blinkMinutes);
}

uint16_t AlarmController::minuteOfDay(uint8_t hour, uint8_t minute) {
	return static_cast<uint16_t>(hour) * kMinutesPerHour + minute;
}

uint16_t AlarmController::minuteOfDay(const DS3231_Wrapper::HoursMinutes& time) {
	int normalizedHour = time.hour % kHoursPerDay;
	if (normalizedHour < 0) {
		normalizedHour += kHoursPerDay;
	}
	int normalizedMinute = time.minute % kMinutesPerHour;
	if (normalizedMinute < 0) {
		normalizedMinute += kMinutesPerHour;
	}
	return minuteOfDay(static_cast<uint8_t>(normalizedHour), static_cast<uint8_t>(normalizedMinute));
}

void AlarmController::refreshGuardForCurrentTime(uint16_t currentMinuteOfDay) {
	for (auto& alarm : alarms_) {
		if (alarm.triggeredThisMinute && alarm.guardMinuteOfDay != currentMinuteOfDay) {
			alarm.triggeredThisMinute = false;
		}
	}
}

void AlarmController::clearGuard(AlarmEntry& alarm) {
	alarm.triggeredThisMinute = false;
	alarm.guardMinuteOfDay = 0;
}

void AlarmController::markModified(AlarmEntry& alarm) {
	alarm.modified = true;
	alarmsDirty_ = true;
}

size_t AlarmController::highestSelectableIndex() const {
	int highestModified = -1;
	for (int i = static_cast<int>(alarms_.size()) - 1; i >= 0; --i) {
		if (alarms_[i].modified) {
			highestModified = i;
			break;
		}
	}
	int limit = highestModified + 1;
	if (limit < 0) {
		limit = 0;
	}
	const int lastIndex = static_cast<int>(alarms_.size()) - 1;
	if (limit > lastIndex) {
		limit = lastIndex;
	}
	return static_cast<size_t>(limit);
}

void AlarmController::loadFromStorage() {
	for (auto& alarm : alarms_) {
		alarm = AlarmEntry{};
	}
	alarmsDirty_ = false;
	if (!settings_) {
		return;
	}
	const size_t limit = std::min(kAlarmCount, PersistentSettings::kMaxStoredAlarms);
	if (limit == 0) {
		return;
	}
	std::array<PersistentSettings::AlarmSetting, PersistentSettings::kMaxStoredAlarms> stored{};
	const size_t loaded = settings_->loadAlarms(stored.data(), limit);
	for (size_t i = 0; i < limit; ++i) {
		auto& alarm = alarms_[i];
		if (i < loaded && stored[i].valid) {
			alarm.hours = stored[i].hour;
			alarm.minutes = stored[i].minute;
			alarm.enabled = stored[i].active;
			alarm.modified = true;
		} else {
			alarm = AlarmEntry{};
		}
		alarm.triggeredThisMinute = false;
		alarm.guardMinuteOfDay = 0;
	}
}

void AlarmController::saveToStorage() {
	if (!settings_ || !alarmsDirty_) {
		return;
	}
	const size_t limit = std::min(kAlarmCount, PersistentSettings::kMaxStoredAlarms);
	if (limit == 0) {
		alarmsDirty_ = false;
		return;
	}
	std::array<PersistentSettings::AlarmSetting, PersistentSettings::kMaxStoredAlarms> snapshot{};
	for (size_t i = 0; i < limit; ++i) {
		auto& slot = snapshot[i];
		const auto& alarm = alarms_[i];
		if (alarm.modified) {
			slot.hour = alarm.hours;
			slot.minute = alarm.minutes;
			slot.active = alarm.enabled;
			slot.valid = true;
		} else {
			slot.hour = 0;
			slot.minute = 0;
			slot.active = false;
			slot.valid = false;
		}
	}
	settings_->storeAlarms(snapshot.data(), limit);
	settings_->saveIfDirty();
	alarmsDirty_ = false;
}
