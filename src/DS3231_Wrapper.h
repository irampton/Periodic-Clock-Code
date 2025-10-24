#ifndef RP2040_DS3231_WRAPPER_H
#define RP2040_DS3231_WRAPPER_H


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

private:
	DS3231 rtc; // Instance of the DS3231 object
	bool h12; // 12-hour mode
	bool hPM; // AM/PM indicator
};


#endif //RP2040_DS3231_WRAPPER_H
