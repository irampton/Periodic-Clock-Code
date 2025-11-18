#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <DS3231.h>
#include <time.h>

class DS3231_Wrapper {
public:
	DS3231_Wrapper(); // Constructor

	struct HoursMinutes {
		int hour;
		int minute;
	};

	HoursMinutes getCachedHoursMinutes(); // Cached hours/minutes fetcher
	void printTime(); // Method to print current time
	int getHours(); // Getter for hours
	int getMinutes(); // Getter for minutes
	int getSeconds(); // Getter for seconds
	uint32_t getEpoch(); // Getter for Unix time (seconds since 1970-01-01)
	void setTime(time_t epochSeconds, int8_t timezoneHourOffset, bool dstEnabled);
	void setTimezoneOffset(int8_t timezoneHourOffset);
	int8_t getTimezoneHourOffset() const;
	void setDstEnabled(bool enabled);
	bool getDstEnabled() const;

private:
	bool isDstActive(const tm &localTime) const;
	int adjustMinutes(int rawMinute, int &hourCarry) const;

	int cachedHour;
	int cachedMinute;
	int lastCachedMinute;
	uint32_t lastMinuteReadMs;

	DS3231 rtc; // Instance of the DS3231 object
	int8_t timezoneHourOffset;   // Hour offset from UTC
	int8_t timezoneMinuteOffset; // Minute offset from UTC
	bool dstEnabled;
};
