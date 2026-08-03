#pragma once

#include <cstdint>

class Display;

// Shared alert output for events that require the user's attention.
class OngoingAlarm {
public:
	enum class Source : uint8_t {
		Alarm,
		Timer,
	};

	void init(Display* display, uint8_t buzzerPin);
	void activate(Source source);
	void dismiss(Source source);
	bool isActive(Source source) const;
	bool isActive() const;
	void tick();

private:
	uint8_t sourceMask_ = 0;
	uint8_t buzzerPin_ = 0;
	Display* display_ = nullptr;
	bool buzzerOn_ = false;
};
