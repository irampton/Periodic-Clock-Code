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
	for (int i = 0; i < numLEDs && i < this->numLEDs; i++)
	{
		leds[i] = colors[i]; // Set each LED's color
	}
	FastLED.show(); // Update the LED strip
}

void LED_Wrapper::setBrightness(uint8_t brightness, bool update) {
	FastLED.setBrightness(brightness);
	if (update) {
		FastLED.show();
	}
}

void LED_Wrapper::fillEvenRainbow(uint8_t startHue) {
	uint8_t delta = 0;
	if (numLEDs > 1) {
		const uint16_t divisor = static_cast<uint16_t>(numLEDs - 1);
		uint8_t step = static_cast<uint8_t>(255 / divisor);
		if (step == 0) {
			step = 1;
		}
		delta = step;
	}
	fill_rainbow(leds, numLEDs, startHue, delta);
}

void LED_Wrapper::renderLEDs() {
	FastLED.show();
}
