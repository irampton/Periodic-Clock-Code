#include "DS3231_Wrapper.h"
#include <Arduino.h>

DS3231_Wrapper::DS3231_Wrapper() {
	// Constructor does not need to initialize anything
}

void DS3231_Wrapper::printTime() {
	int hour = DS3231_Wrapper::rtc.getHour(h12, hPM); // Get hour in 24-hour format
	int minute = DS3231_Wrapper::rtc.getMinute();
	int second = DS3231_Wrapper::rtc.getSecond();

	// Print time in HH:MM:SS format
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

int DS3231_Wrapper::getHours() {
	return DS3231_Wrapper::rtc.getHour(h12, hPM); // Return the hour
}

int DS3231_Wrapper::getMinutes() {
	return DS3231_Wrapper::rtc.getMinute(); // Return the minute
}

int DS3231_Wrapper::getSeconds() {
	return DS3231_Wrapper::rtc.getSecond(); // Return the second
}

void DS3231_Wrapper::setHours(uint8_t hour) {
	if (hour > 23) {
		return;
	}
	rtc.setHour(hour);
}

void DS3231_Wrapper::setMinutes(uint8_t minute) {
	if (minute > 59) {
		return;
	}
	rtc.setMinute(minute);
}

void DS3231_Wrapper::setSeconds(uint8_t second) {
	if (second > 59) {
		return;
	}
	rtc.setSecond(second);
}
