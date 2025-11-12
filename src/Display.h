#pragma once

#include <string>
#include <algorithm>
#include <cstring>
#include <vector>
#include <cstdint>

#include "LED_Wrapper.h"
#include "font.h"


class Display {
public:
	Display(int rows, int cols, int led_pin);
	void init();
	void write_string(char text[], CRGB* colors, bool fade = false);
	void write_string(const std::string& text, CRGB* colors, bool fade = false);
	void incrementBrightness();
	void decrementBrightness();
   	void tick();

private:
	uint8_t height;
	uint8_t width;
	uint8_t brightness;
	bool newData;
	bool fadeActive;
	uint32_t fadeStartMillis;
	LED_Wrapper driver;
	std::vector<CRGB> displayedFrame;
	std::vector<CRGB> targetFrame;
	std::vector<CRGB> fadeFromFrame;
	void applyFrame(const std::vector<CRGB>& frame);
	void renderFadeFrame(float progress);
};
