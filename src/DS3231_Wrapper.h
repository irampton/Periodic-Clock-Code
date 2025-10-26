#ifndef RP2040_DS3231_WRAPPER_H
#define RP2040_DS3231_WRAPPER_H


#include <Arduino.h>
#include <Wire.h>
#include <DS3231.h>

class DS3231_Wrapper
{
public:
	DS3231_Wrapper(); // Constructor

	void printTime(); // Method to print current time
	int getHours(); // Getter for hours
	int getMinutes(); // Getter for minutes
	int getSeconds(); // Getter for seconds
	void setHours(uint8_t hour); // Setter for hours
	void setMinutes(uint8_t minute); // Setter for minutes
	void setSeconds(uint8_t second); // Setter for seconds

private:
	DS3231 rtc; // Instance of the DS3231 object
	bool h12; // 12-hour mode
	bool hPM; // AM/PM indicator
};


#endif //RP2040_DS3231_WRAPPER_H
