#include "Rotary.h"
#include <Arduino.h>

// Timing control
unsigned long lastUpdate = 0;
const unsigned long interval = 1; // Run state machine every 10ms

// Constructor to initialize pin numbers
Rotary::Rotary(int CW_pin, int CCW_pin, int BTN_pin)
	: CW_pin(CW_pin), CCW_pin(CCW_pin), BTN_pin(BTN_pin) {
	Rotary::currentState = Rotary_START;
}

void Rotary::init() {
	// Set pins 14 and 15 to input_pullup mode
	pinMode(CW_pin, INPUT_PULLUP);
	pinMode(CCW_pin, INPUT_PULLUP);
	pinMode(BTN_pin, INPUT_PULLUP);
}

void Rotary::loop(bool input[3]) {
	input[0] = false;
	input[1] = false;
	input[2] = false;
	// Check if 10ms has passed (for timing control)
	if (millis() - lastUpdate >= interval)
	{
		lastUpdate = millis(); // Update last update time

		// Read the state of pins 14 and 15
		bool CWPinLow = digitalRead(Rotary::CW_pin) == LOW;
		bool CCWPinLow = digitalRead(Rotary::CCW_pin) == LOW;
		bool BTNPinLow = digitalRead(Rotary::BTN_pin) == LOW;

		// State machine logic
		switch (currentState)
		{
		case Rotary_START:
			// Handle low readings for CW and CCW transitions
			if (CWPinLow)
			{
				Rotary::countCW++;
			} else
			{
				Rotary::countCW = 0; // Reset count if pin 14 is high
			}

			if (CCWPinLow)
			{
				Rotary::countCCW++;
			} else
			{
				Rotary::countCCW = 0; // Reset count if pin 15 is high
			}

			if (BTNPinLow)
			{
				Rotary::countBTN++;
			} else
			{
				Rotary::countBTN = 0; // Reset count if pin 15 is high
			}

		// Transition to CCW if both 14 and 15 are low for 2 consecutive readings
			if (Rotary::countCCW >= 2)
			{
				currentState = Rotary_CCW;
				input[0] = true;
				Rotary::countCW = 0;
				Rotary::countCCW = 0;
				Rotary::countBTN = 0;
			}

		// Transition to CW if both 14 and 15 are low for 2 consecutive readings
			if (Rotary::countCW >= 2)
			{
				currentState = Rotary_CW;
				input[1] = true;
				Rotary::countCW = 0;
				Rotary::countCCW = 0;
				Rotary::countBTN = 0;
			}

			if (Rotary::countBTN >= 2)
			{
				currentState = Rotary_BTN;
				input[2] = true;
				Rotary::countCW = 0;
				Rotary::countCCW = 0;
				Rotary::countBTN = 0;
			}

			break;

		case Rotary_CW:
			// Handle high readings for returning to START
			if (!CWPinLow && !CCWPinLow)
			{
				highCount++;
			} else
			{
				highCount = 0; // Reset count if either pin is low
			}

		// Transition back to START if both pins are high for 4 consecutive readings
			if (highCount >= 4)
			{
				currentState = Rotary_START;
				highCount = 0; // Reset high count after transition
			}

			break;

		case Rotary_CCW:

			// Handle high readings for returning to START
			if (!CWPinLow && !CCWPinLow)
			{
				highCount++;
			} else
			{
				highCount = 0; // Reset count if either pin is low
			}

		// Transition back to START if both pins are high for 4 consecutive readings
			if (highCount >= 4)
			{
				currentState = Rotary_START;
				highCount = 0; // Reset high count after transition
			}

			break;

		case Rotary_BTN:
			if (!BTNPinLow)
			{
				highCount++;
			} else
			{
				highCount = 0; // Reset count if either pin is low
			}
		// Transition back to START if both pins are high for 4 consecutive readings
			if (highCount >= 4)
			{
				currentState = Rotary_START;
				highCount = 0; // Reset high count after transition
			}
			break;
		}
	}
}
