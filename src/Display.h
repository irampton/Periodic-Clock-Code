#pragma once
#include "LED_Wrapper.h"
#include "font.h"


class Display {
public:
	Display(int rows, int cols, int led_pin);
	void init();
	void write_string(char text[], CRGB* colors);
	void incrementBrightness();
	void decrementBrightness();

private:
	int height;
	int width;
	uint8_t brightness;
	LED_Wrapper driver;
};
