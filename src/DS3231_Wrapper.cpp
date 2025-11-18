#include "DS3231_Wrapper.h"
#include <Arduino.h>
#include <string.h>

namespace {
	constexpr int8_t MIN_TIMEZONE_HOURS = -12; // UTC-12
	constexpr int8_t MAX_TIMEZONE_HOURS = 14; // UTC+14

	constexpr uint8_t daysInMonth(uint16_t year, uint8_t month) {
		const uint8_t days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
		if (month == 0 || month > 12) return 30;
		uint8_t base = days[month - 1];
		// leap year check for February
		if (month == 2) {
			const bool leap = ((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0);
			return base + (leap ? 1 : 0);
		}
		return base;
	}

	constexpr uint8_t weekday(uint16_t year, uint8_t month, uint8_t day) {
		// Sakamoto's algorithm; returns 0=Sunday .. 6=Saturday
		const uint8_t t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
		if (month < 3) {
			--year;
		}
		return static_cast<uint8_t>((year + year / 4 - year / 100 + year / 400 + t[month - 1] + day) % 7);
	}
}

DS3231_Wrapper::DS3231_Wrapper()
	: cachedHour(-1), cachedMinute(-1),lastCachedMinute(-1), lastMinuteReadMs(0),
	  timezoneHourOffset(0), timezoneMinuteOffset(0), dstEnabled(false) {
}

void DS3231_Wrapper::printTime() {
	const int hour = getHours();
	const int minute = getMinutes();
	const int second = getSeconds();

	Serial.print("Current Time: ");
	if (hour < 10) Serial.print("0");
	Serial.print(hour);
	Serial.print(":");
	if (minute < 10) Serial.print("0");
	Serial.print(minute);
	Serial.print(":");
	if (second < 10) Serial.print("0");
	Serial.println(second);
}

DS3231_Wrapper::HoursMinutes DS3231_Wrapper::getCachedHoursMinutes() {
	const uint32_t nowMs = millis();
	constexpr uint32_t MINUTE_REFRESH_WINDOW_MS = 500; // Read minute once or twice a second

	// Longer than 30 minutes between calls
	const bool long_wait_between_calls = nowMs - lastMinuteReadMs > 1000 * 60 * 29;

	if (cachedMinute < 0 || nowMs - lastMinuteReadMs >= MINUTE_REFRESH_WINDOW_MS) {
		lastCachedMinute = cachedMinute;
		cachedMinute = rtc.getMinute();
		lastMinuteReadMs = nowMs;
	}


	if (cachedHour < 0 || cachedMinute < lastCachedMinute || long_wait_between_calls) {
		cachedHour = getHours();
		lastCachedMinute = cachedMinute;
	}

	return {cachedHour, cachedMinute};
}

int DS3231_Wrapper::getHours() {
	bool h12 = false;
	bool hPM = false;
	const int rawHour = rtc.getHour(h12, hPM);
	const int rawDay = rtc.getDate();
	bool century = false;
	const int rawMonth = rtc.getMonth(century);
	const int rawYear = rtc.getYear(); // two-digit year

	int localHour = rawHour + timezoneHourOffset;
	int dayCarry = 0;
	while (localHour < 0) {
		localHour += 24;
		--dayCarry;
	}
	while (localHour >= 24) {
		localHour -= 24;
		++dayCarry;
	}

	int year = 2000 + rawYear; // DS3231 returns 0-99
	int month = rawMonth;
	int day = rawDay;

	if (dayCarry != 0) {
		int carry = dayCarry;
		while (carry != 0) {
			if (carry > 0) {
				const uint8_t dim = daysInMonth(static_cast<uint16_t>(year), static_cast<uint8_t>(month));
				++day;
				if (day > static_cast<int>(dim)) {
					day = 1;
					++month;
					if (month > 12) {
						month = 1;
						++year;
					}
				}
				--carry;
			} else {
				--day;
				if (day < 1) {
					--month;
					if (month < 1) {
						month = 12;
						--year;
					}
					day = daysInMonth(static_cast<uint16_t>(year), static_cast<uint8_t>(month));
				}
				++carry;
			}
		}
	}

	if (dstEnabled) {
		tm localTm{};
		localTm.tm_year = year - 1900;
		localTm.tm_mon = month - 1;
		localTm.tm_mday = day;
		localTm.tm_hour = localHour;
		localTm.tm_min = 0;
		localTm.tm_sec = 0;
		localTm.tm_wday = weekday(static_cast<uint16_t>(year), static_cast<uint8_t>(month), static_cast<uint8_t>(day));

		if (isDstActive(localTm)) {
			localHour += 1;
			if (localHour >= 24) {
				localHour -= 24;
				// minute offset cannot push further day change here since only DST hour add
			}
		}
	}

	if (localHour < 0) {
		localHour += 24;
	} else if (localHour >= 24) {
		localHour %= 24;
	}
	return localHour;
}

int DS3231_Wrapper::getMinutes() {
	return rtc.getMinute();
}

int DS3231_Wrapper::getSeconds() {
	return rtc.getSecond();
}

uint32_t DS3231_Wrapper::getEpoch() {
	const DateTime now = RTClib::now();
	return now.unixtime();
}

void DS3231_Wrapper::setTime(time_t epochSeconds, int8_t timezoneHour, bool dstEnabledFlag) {
	if (timezoneHour < MIN_TIMEZONE_HOURS) {
		timezoneHour = MIN_TIMEZONE_HOURS;
	} else if (timezoneHour > MAX_TIMEZONE_HOURS) {
		timezoneHour = MAX_TIMEZONE_HOURS;
	}

	setTimezoneOffset(timezoneHour);
	dstEnabled = dstEnabledFlag;

	rtc.setClockMode(false); // Force 24-hour mode
	rtc.setEpoch(epochSeconds, false); // Store RTC as GMT
	cachedHour = -1;
	cachedMinute = -1;
	lastCachedMinute = -1;
}

void DS3231_Wrapper::setTimezoneOffset(int8_t timezoneHour) {
	if (timezoneHour < MIN_TIMEZONE_HOURS) {
		timezoneHour = MIN_TIMEZONE_HOURS;
	} else if (timezoneHour > MAX_TIMEZONE_HOURS) {
		timezoneHour = MAX_TIMEZONE_HOURS;
	}

	timezoneHourOffset = timezoneHour;
}

int8_t DS3231_Wrapper::getTimezoneHourOffset() const {
	return timezoneHourOffset;
}

bool DS3231_Wrapper::isDstActive(const tm& localTime) const {
	const int month = localTime.tm_mon + 1; // 1-12
	const int day = localTime.tm_mday;
	const int hour = localTime.tm_hour;
	const int wday = localTime.tm_wday; // 0 = Sunday

	if (month < 3 || month > 11) {
		return false;
	}
	if (month > 3 && month < 11) {
		return true;
	}

	// Calculate weekday for the first of the month based on current day
	const int wdayFirst = (wday - ((day - 1) % 7) + 7) % 7;
	const int firstSunday = (wdayFirst == 0) ? 1 : (8 - wdayFirst);

	if (month == 3) {
		const int secondSunday = firstSunday + 7;
		if (day > secondSunday) {
			return true;
		}
		if (day < secondSunday) {
			return false;
		}
		return hour >= 2; // Starts at 2:00 local time
	}

	// month == 11
	if (day < firstSunday) {
		return true;
	}
	if (day > firstSunday) {
		return false;
	}
	return hour < 2; // Ends at 2:00 local time
}
