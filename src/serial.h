#pragma once

#include <Arduino.h>
#include <stdlib.h>

#include "DS3231_Wrapper.h"

inline bool parseLong(const String &value, long &out) {
	if (value.length() == 0) {
		return false;
	}
	char *endPtr = nullptr;
	out = strtol(value.c_str(), &endPtr, 10);
	return endPtr != nullptr && *endPtr == '\0';
}

inline void handleSerialCommand(const String &command, DS3231_Wrapper &myRTC) {
	String trimmed = command;
	trimmed.trim();
	if (trimmed.length() == 0) {
		return;
	}

	String lowered = trimmed;
	lowered.toLowerCase();

	if (!lowered.startsWith("set ")) {
		return;
	}

	String remainder = trimmed.substring(4);
	remainder.trim();
	const int separator = remainder.indexOf(' ');
	if (separator < 0) {
		Serial.println("Invalid set command; expected format 'set <target> ...'");
		return;
	}

	String target = remainder.substring(0, separator);
	target.toLowerCase();
	String args = remainder.substring(separator + 1);
	args.trim();

	if (target != "time") {
		Serial.print("Unknown set target: ");
		Serial.println(target);
		return;
	}

	const int firstSep = args.indexOf(' ');
	const int secondSep = (firstSep < 0) ? -1 : args.indexOf(' ', firstSep + 1);
	if (firstSep < 0 || secondSep < 0 ) {
		Serial.println("Invalid set time command; expected 'set time <epoch> <hour_offset> <dst_flag>'");
		return;
	}

	String epochToken = args.substring(0, firstSep);
	epochToken.trim();
	String hourToken = args.substring(firstSep + 1, secondSep);
	hourToken.trim();
	String dstToken = args.substring(secondSep + 1);
	dstToken.trim();

	long epochLong = 0;
	long hourLong = 0;
	long dstLong = 0;
	if (!parseLong(epochToken, epochLong) || !parseLong(hourToken, hourLong) || !parseLong(dstToken, dstLong)) {
		Serial.println("Set time requires numeric epoch, hour offset, and DST flag (0/1)");
		return;
	}

	myRTC.setTime(static_cast<time_t>(epochLong), static_cast<int8_t>(hourLong), dstLong != 0);

	Serial.print("Time set to epoch ");
	Serial.print(epochLong);
	Serial.print(" with timezone hour offset ");
	Serial.print(hourLong);
	Serial.print(", DST ");
	Serial.println(dstLong != 0 ? "enabled" : "disabled");
}
