#include "LED_Wrapper.h"

LED_Wrapper::LED_Wrapper(int pin, int numLEDs) {
	this->pin = pin; // Set the pin number
	this->numLEDs = numLEDs; // Set the number of LEDs
	leds = new CRGB[numLEDs]; // Dynamically allocate memory for the LEDs
}

void LED_Wrapper::init() {
	FastLED.addLeds<NEOPIXEL, 15>(leds, numLEDs); // Initialize the LED strip
	FastLED.setBrightness(255); // Set the maximum brightness
}

void LED_Wrapper::clearAll() {
	fill_solid(leds, numLEDs, CRGB::Black); // Fill the entire strip with black (turn off all LEDs)
	FastLED.show(); // Update the LED strip
}

void LED_Wrapper::setLED(int index, CRGB color) {
	if (index >= 0 && index < numLEDs)
	{ // Ensure the index is valid (0-based index)
		leds[index] = color; // Set the color of the LED at the given index
		FastLED.show(); // Update the LED strip
	}
}

void LED_Wrapper::preSetLED(int index, CRGB color) {
	if (index >= 0 && index < numLEDs)
	{ // Ensure the index is valid (0-based index)
		leds[index] = color; // Set the color of the LED at the given index
	}
}

void LED_Wrapper::setColors(CRGB* colors, int numLEDs) {
	if (numLEDs == this->numLEDs)
	{ // Ensure the array size matches the number of LEDs
		for (int i = 0; i < numLEDs; i++)
		{
			leds[i] = colors[i]; // Set each LED's color
		}
		FastLED.show(); // Update the LED strip
	}
}

void LED_Wrapper::renderLEDs() {
	FastLED.show();
}
