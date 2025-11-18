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

private:
	static constexpr uint32_t kMagic = 0x70504344; // 'pPCD'
	static constexpr uint16_t kVersion = 1;

	// Pad this struct for future expansion; keep it packed for predictable layout.
	struct __attribute__((packed)) SettingsBlob {
		uint32_t magic;
		uint16_t version;
		uint8_t timeMode;
		int8_t timezoneOffsetHours;
		uint8_t dstEnabled;
		uint8_t reserved[12];
	};

	static constexpr size_t kStorageSize = sizeof(SettingsBlob);

	SettingsBlob data_{};
	bool dirty_ = false;

	void loadFromEeprom();
	void writeToEeprom();
	static SettingsBlob defaultSettings();
	static TimeMode toTimeMode(uint8_t rawMode);
};
