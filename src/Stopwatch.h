#pragma once

#include <cstdint>

class Stopwatch {
public:
	Stopwatch();

	void init();
	void start();
	void stop();
	void reset();
	void getTime(uint8_t *hours, uint8_t *minutes, uint8_t *seconds);
	bool isRunning() const;
	void toggle();

private:
	bool initialized;
	bool running;
	uint32_t startMillis;
	uint32_t elapsedMillis;
};
