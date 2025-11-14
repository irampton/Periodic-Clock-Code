#pragma once

inline bool isUnsignedInteger(const String& value) {
	if (value.length() == 0) {
		return false;
	}

	for (size_t i = 0; i < value.length(); ++i) {
		const char c = value.charAt(i);
		if (c < '0' || c > '9') {
			return false;
		}
	}

	return true;
}

inline void handleSerialCommand(const String& command, DS3231_Wrapper myRTC) {
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

	String remainder = lowered.substring(4);
	remainder.trim();
	const int separator = remainder.indexOf(' ');
	if (separator < 0) {
		Serial.println("Invalid set command; expected format 'set <target> <value>'");
		return;
	}

	const String target = remainder.substring(0, separator);
	String valueToken = remainder.substring(separator + 1);
	valueToken.trim();

	if (!isUnsignedInteger(valueToken)) {
		Serial.println("Value must be numeric");
		return;
	}

	const int value = valueToken.toInt();

	if (target == "hour") {
		if (value >= 0 && value <= 23) {
			myRTC.setHours(static_cast<uint8_t>(value));
			Serial.print("Hour set to ");
			Serial.println(value);
		} else {
			Serial.println("Hour must be between 0 and 23");
		}
	} else if (target == "min" || target == "mins" || target == "minute" || target == "minutes") {
		if (value >= 0 && value <= 59) {
			myRTC.setMinutes(static_cast<uint8_t>(value));
			Serial.print("Minute set to ");
			Serial.println(value);
		} else {
			Serial.println("Minute must be between 0 and 59");
		}
	} else if (target == "sec" || target == "secs" || target == "second" || target == "seconds") {
		if (value >= 0 && value <= 59) {
			myRTC.setSeconds(static_cast<uint8_t>(value));
			Serial.print("Second set to ");
			Serial.println(value);
		} else {
			Serial.println("Second must be between 0 and 59");
		}
	} else {
		Serial.print("Unknown set target: ");
		Serial.println(target);
	}
}
