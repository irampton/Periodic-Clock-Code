#include <Arduino.h>
#include <Wire.h>

#include "DS3231_Wrapper.h"
#include "LED_Wrapper.h"

// ---------- Setup / Loop ----------
void setup() {
	Wire.setSDA(0);
	Wire.setSCL(1);
	Wire.begin();

	Serial.begin(9600);
}

void loop() {
}

void setup1() {
}

void loop1() {
}
