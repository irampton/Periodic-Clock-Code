#pragma once

#include <Arduino.h>
#include <EEPROM.h>
#include <cstddef>

#include "clockText.h"

// Handles loading and saving small configuration values to EEPROM/flash.
// Writes are deferred until explicitly requested to avoid wearing the part.
class PersistentSettings {
public:
	void begin();

	TimeMode getTimeMode() const;
	void setTimeMode(TimeMode mode, bool saveImmediately = false);

	int8_t getTimezoneOffsetHours() const;
	void setTimezoneOffsetHours(int8_t offsetHours, bool saveImmediately = false);

	bool isDstEnabled() const;
	void setDstEnabled(bool enabled, bool saveImmediately = false);

	// Saves the pending changes if any have been made.
	void saveIfDirty();
	// Force an immediate save (useful for rare operations, e.g. setting the time).
	void saveNow();

	struct AlarmSetting {
		uint8_t hour = 0;
		uint8_t minute = 0;
		bool active = false;
		bool valid = false;
	};

	static constexpr size_t kMaxStoredAlarms = 10;
	size_t loadAlarms(AlarmSetting* alarms, size_t maxAlarms) const;
	void storeAlarms(const AlarmSetting* alarms, size_t count);

private:
	static constexpr uint32_t kMagic = 0x70504344; // 'pPCD'
	static constexpr uint16_t kVersion = 2;
	static constexpr uint8_t kUnusedAlarmHour = 0xFF;

	// Pad this struct for future expansion; keep it packed for predictable layout.
	struct __attribute__((packed)) SettingsBlob {
		uint32_t magic;
		uint16_t version;
		uint8_t timeMode;
		int8_t timezoneOffsetHours;
		uint8_t dstEnabled;
		uint8_t reserved[8];
		struct __attribute__((packed)) AlarmRecord {
			uint8_t hour;
			uint8_t minute;
			uint8_t active;
		};
		AlarmRecord alarms[kMaxStoredAlarms];
	};

	static constexpr size_t kStorageSize = sizeof(SettingsBlob);

	SettingsBlob data_{};
	bool dirty_ = false;

	void loadFromEeprom();
	void writeToEeprom();
	static SettingsBlob defaultSettings();
	static TimeMode toTimeMode(uint8_t rawMode);
};
