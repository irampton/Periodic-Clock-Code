#pragma once

#include <FastLED.h>

class LED_Wrapper {
public:
	LED_Wrapper(int pin, int numLEDs); // Constructor takes pin number and number of LEDs

	void init(); // Initialize FastLED
	void clearAll(); // Clear all LEDs
	void setLED(int index, CRGB color); // Set a specific LED based on its index (0-based index)
	void preSetLED(int index, CRGB color); // Set a specific LED based on its index without showing it
	void setColors(CRGB* colors, int numLEDs); // Set the colors of multiple LEDs based on an array
	void fillEvenRainbow(uint8_t startHue = 0);
	void setBrightness(uint8_t brightness, bool update = true);
	void renderLEDs(); // Shows the LEDs

private:
	int pin; // Pin number where the LED strip is connected
	int numLEDs; // Number of LEDs in the strip
	CRGB* leds; // Array to hold LED colors
};
