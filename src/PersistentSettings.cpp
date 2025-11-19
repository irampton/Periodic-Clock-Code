#include "PersistentSettings.h"

#include <algorithm>
#include <cstring>

void PersistentSettings::begin() {
#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_RP2040) || defined(ARDUINO_ARCH_STM32)
	EEPROM.begin(static_cast<int>(kStorageSize));
#endif
	loadFromEeprom();
}

TimeMode PersistentSettings::getTimeMode() const {
	return toTimeMode(data_.timeMode);
}

void PersistentSettings::setTimeMode(TimeMode mode, bool saveImmediately) {
	const uint8_t rawMode = static_cast<uint8_t>(mode);
	if (data_.timeMode != rawMode) {
		data_.timeMode = rawMode;
		dirty_ = true;
	}
	if (saveImmediately) {
		saveIfDirty();
	}
}

int8_t PersistentSettings::getTimezoneOffsetHours() const {
	return data_.timezoneOffsetHours;
}

void PersistentSettings::setTimezoneOffsetHours(int8_t offsetHours, bool saveImmediately) {
	if (data_.timezoneOffsetHours != offsetHours) {
		data_.timezoneOffsetHours = offsetHours;
		dirty_ = true;
	}
	if (saveImmediately) {
		saveIfDirty();
	}
}

bool PersistentSettings::isDstEnabled() const {
	return data_.dstEnabled != 0;
}

void PersistentSettings::setDstEnabled(bool enabled, bool saveImmediately) {
	const uint8_t value = enabled ? 1 : 0;
	if (data_.dstEnabled != value) {
		data_.dstEnabled = value;
		dirty_ = true;
	}
	if (saveImmediately) {
		saveIfDirty();
	}
}

void PersistentSettings::saveIfDirty() {
	if (dirty_) {
		writeToEeprom();
	}
}

void PersistentSettings::saveNow() {
	saveIfDirty();
}

size_t PersistentSettings::loadAlarms(AlarmSetting* alarms, size_t maxAlarms) const {
	if (!alarms) {
		return 0;
	}
	const size_t limit = std::min(maxAlarms, kMaxStoredAlarms);
	for (size_t i = 0; i < limit; ++i) {
		const auto& stored = data_.alarms[i];
		if (stored.hour == kUnusedAlarmHour) {
			alarms[i] = AlarmSetting{};
			continue;
		}
		alarms[i].hour = stored.hour;
		alarms[i].minute = stored.minute;
		alarms[i].active = stored.active != 0;
		alarms[i].valid = true;
	}
	return limit;
}

void PersistentSettings::storeAlarms(const AlarmSetting* alarms, size_t count) {
	if (!alarms) {
		return;
	}
	const size_t limit = std::min(count, kMaxStoredAlarms);
	bool changed = false;
	for (size_t i = 0; i < kMaxStoredAlarms; ++i) {
		SettingsBlob::AlarmRecord record{};
		if (i < limit && alarms[i].valid) {
			record.hour = alarms[i].hour;
			record.minute = alarms[i].minute;
			record.active = alarms[i].active ? 1 : 0;
		} else {
			record.hour = kUnusedAlarmHour;
			record.minute = 0;
			record.active = 0;
		}
		if (record.hour != data_.alarms[i].hour || record.minute != data_.alarms[i].minute ||
		    record.active != data_.alarms[i].active) {
			data_.alarms[i] = record;
			changed = true;
		}
	}
	if (changed) {
		dirty_ = true;
	}
}

PersistentSettings::SettingsBlob PersistentSettings::defaultSettings() {
	SettingsBlob blob{};
	blob.magic = kMagic;
	blob.version = kVersion;
	blob.timeMode = static_cast<uint8_t>(TimeMode::Hour24);
	blob.timezoneOffsetHours = 0;
	blob.dstEnabled = 0;
	memset(blob.reserved, 0, sizeof(blob.reserved));
	for (auto& alarm : blob.alarms) {
		alarm.hour = kUnusedAlarmHour;
		alarm.minute = 0;
		alarm.active = 0;
	}
	return blob;
}

TimeMode PersistentSettings::toTimeMode(uint8_t rawMode) {
	switch (static_cast<TimeMode>(rawMode)) {
		case TimeMode::Hour12:
			return TimeMode::Hour12;
		case TimeMode::Hour24:
			return TimeMode::Hour24;
	}
	return TimeMode::Hour24;
}

void PersistentSettings::loadFromEeprom() {
	SettingsBlob stored{};
	EEPROM.get(0, stored);

	if (stored.magic != kMagic || stored.version != kVersion) {
		data_ = defaultSettings();
		writeToEeprom();
		return;
	}

	data_ = stored;
	dirty_ = false;
}

void PersistentSettings::writeToEeprom() {
	EEPROM.put(0, data_);
#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_RP2040) || defined(ARDUINO_ARCH_STM32)
	EEPROM.commit();
#endif
	dirty_ = false;
}
