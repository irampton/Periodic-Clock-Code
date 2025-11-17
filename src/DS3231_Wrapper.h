#ifndef RP2040_DS3231_WRAPPER_H
#define RP2040_DS3231_WRAPPER_H

#include <Arduino.h>
#include <Wire.h>
#include <DS3231.h>
#include <time.h>

class DS3231_Wrapper {
public:
	DS3231_Wrapper(); // Constructor

	void printTime(); // Method to print current time
	int getHours(); // Getter for hours
	int getMinutes(); // Getter for minutes
	int getSeconds(); // Getter for seconds
	void setTime(time_t epochSeconds, int8_t timezoneHourOffset, bool dstEnabled);
	void setTimezoneOffset(int8_t timezoneHourOffset);
	int8_t getTimezoneHourOffset() const;

private:
	bool isDstActive(const tm &localTime) const;
	int adjustMinutes(int rawMinute, int &hourCarry) const;

	DS3231 rtc; // Instance of the DS3231 object
	int8_t timezoneHourOffset;   // Hour offset from UTC
	int8_t timezoneMinuteOffset; // Minute offset from UTC
	bool dstEnabled;
};

#endif //RP2040_DS3231_WRAPPER_H
