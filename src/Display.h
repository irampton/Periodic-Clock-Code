#pragma once
#include "LED_Wrapper.h"
#include "font.h"


class Display {
public:
	Display(int rows, int cols, int led_pin);
	void init();
	void write_string(char text[], CRGB* colors);

private:
	int height;
	int width;
	LED_Wrapper driver;
};
