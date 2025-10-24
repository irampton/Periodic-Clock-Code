#ifndef RP2040_ROTARY_H
#define RP2040_ROTARY_H

// Define state identifiers
enum Rotary_State
{
	Rotary_START,
	Rotary_CW,
	Rotary_CCW,
	Rotary_BTN
};

class Rotary
{
private:
	// Member variables to store the pin numbers
	int CW_pin;
	int CCW_pin;
	int BTN_pin;

	Rotary_State currentState;

	// Count consecutive low or high readings
	int countCW = 0;
	int countCCW = 0;
	int countBTN = 0;
	int highCount = 0;

public:
	// Constructor to initialize pin numbers
	Rotary(int CW_pin, int CCW_pin, int BTN_pin);

	// Init the Rotary
	void init();

	// Loop for the Rotary
	// Pass in a bool[3] and get out any pins.
	void loop(bool input[3]);
};


#endif //RP2040_ROTARY_H
