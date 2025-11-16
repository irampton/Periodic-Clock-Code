#include <Arduino.h>
#include <Wire.h>

#include "Display.h"
#include "DS3231_Wrapper.h"
#include "Stopwatch.h"

#include "Rotary.h"
#include "serial.h"
#include "InputEventBuffer.h"
#include "clockText.h"

#define ROWS 7
#define COLUMNS 27
#define LED_PIN 15

#define BUZZER_PIN 13

Rotary* r1 = new Rotary(9, 10, 8);
DS3231_Wrapper myRTC;
Stopwatch stopwatch;
Display* display = new Display(ROWS, COLUMNS, LED_PIN);
ClockText* clockTextFormatter = new ClockText();

enum class CurrentMode {
	clock,
	stopwatch,
	timer,
	alarm,
	settings,
};

NumberDisplayMode number_display_mode = NumberDisplayMode::Periodic;
CurrentMode current_mode = CurrentMode::clock;

const char* inputEventTypeToString(InputEventType type) {
	switch (type) {
		case InputEventType::Pressed:
			return "Pressed";
		case InputEventType::DoubleClick:
			return "DoubleClick";
		case InputEventType::Hold:
			return "Hold";
	}
	return "Unknown";
}

void logButtonEvent(const char* label, InputEventType type) {
	Serial.print(label);
	Serial.print(" ");
	Serial.println(inputEventTypeToString(type));
}

void setup() {
	Wire.setSDA(0);
	Wire.setSCL(1);
	Wire.begin();

	display->init();
	InputEventBuffer::init();

	pinMode(BUZZER_PIN, OUTPUT);

	Serial.begin(9600);

	stopwatch.init(myRTC);
}

std::string clock_text;
CRGB clock_colors[CLOCK_LENGTH];

void loop() {
	uint32_t startTime = millis();

	// Check for anything over the serial port
	// This is used to set the time programmatically
	static String serialBuffer;
	while (Serial.available() > 0) {
		const char incoming = static_cast<char>(Serial.read());
		if (incoming == '\r' || incoming == '\n') {
			if (serialBuffer.length() > 0) {
				handleSerialCommand(serialBuffer, myRTC);
				serialBuffer = "";
			}
		} else if (serialBuffer.length() < 32) {
			serialBuffer += incoming;
		}
	}

	// Go through any input that loop1() picked up
	static bool displayInitialised = false;
	InputEvent event{};
	while (InputEventBuffer::pop(event)) {
		switch (event.type) {
			//switch modes on a double click
			case InputEventType::DoubleClick:
				switch (event.key) {
					case InputKey::AuxButton0:
						if (current_mode != CurrentMode::clock) {
							current_mode = CurrentMode::clock;
							displayInitialised = false;
						}
						break;
					case InputKey::AuxButton1:
						if (current_mode != CurrentMode::stopwatch) {
							current_mode = CurrentMode::stopwatch;
							displayInitialised = false;
						}
						break;
					case InputKey::AuxButton2:
						if (current_mode != CurrentMode::timer) {
							current_mode = CurrentMode::timer;
							displayInitialised = false;
						}
						break;
					case InputKey::AuxButton3:
						if (current_mode != CurrentMode::alarm) {
							current_mode = CurrentMode::alarm;
							displayInitialised = false;
						}
						break;
					case InputKey::AuxButton4:
						if (current_mode != CurrentMode::settings) {
							current_mode = CurrentMode::settings;
							displayInitialised = false;
						}
						break;
					default:
						break;
				}
				break;
			case InputEventType::Pressed:
				switch (event.key) {
					case InputKey::RotaryCW:
						if (event.type == InputEventType::Pressed) {
							display->incrementBrightness();
							Serial.println("RotaryCW");
						}
						break;
					case InputKey::RotaryCCW:
						if (event.type == InputEventType::Pressed) {
							display->decrementBrightness();
							Serial.println("RotaryCCW");
						}
						break;
					case InputKey::RotaryButton:
						logButtonEvent("RotaryButton", event.type);
						break;
					case InputKey::AuxButton0:
						switch (number_display_mode) {
							case NumberDisplayMode::Periodic:
								number_display_mode = NumberDisplayMode::Numeral;
								break;
							case NumberDisplayMode::Numeral:
								number_display_mode = NumberDisplayMode::Periodic;
								break;
						}
						if (current_mode == CurrentMode::stopwatch) {
							displayInitialised = false;
						}
						break;
					case InputKey::AuxButton1:
						switch (current_mode) {
							case CurrentMode::stopwatch:
								stopwatch.toggle();
								break;
							default:
								break;
						}
						break;
					case InputKey::AuxButton2:
						break;
					case InputKey::AuxButton3:
						break;
					case InputKey::AuxButton4:
						break;
				}
				break;
			case InputEventType::Hold:
				switch (event.key) {
					case InputKey::AuxButton0:
						break;
					case InputKey::AuxButton1:
						switch (current_mode) {
							case CurrentMode::stopwatch:
								stopwatch.stop();
								stopwatch.reset();
								break;
							default:
								break;
						}
						break;
					case InputKey::AuxButton2:
						break;
					case InputKey::AuxButton3:
						break;
					case InputKey::AuxButton4:
						break;
					default:
						break;
				}
				break;
		}
	}

	if (InputEventBuffer::consumeOverflowFlag()) {
		Serial.println("Input queue overflow");
	}

	// Update the screen, if necessary
	switch (current_mode) {
		case CurrentMode::clock: {
			// Decide if we need to update the clock, then update it
			static int displayedHours = -1;
			static int displayedMinutes = -1;
			static NumberDisplayMode displayedMode = NumberDisplayMode::Periodic;

			int hours = myRTC.getHours();
			const int minutes = myRTC.getMinutes();

			const bool timeChanged = (hours != displayedHours) || (minutes != displayedMinutes);
			const bool modeChanged = (number_display_mode != displayedMode);

			if (!displayInitialised || timeChanged || modeChanged) {
				clockTextFormatter->prepareTimeString(hours, minutes, number_display_mode, clock_text, clock_colors);

				display->write_string(clock_text, clock_colors, true);
				displayedHours = hours;
				displayedMinutes = minutes;
				displayedMode = number_display_mode;
				displayInitialised = true;
			}
			break;
		}
		case CurrentMode::stopwatch: {
			static uint8_t displayedHigh = 255;
			static uint8_t displayedLow = 255;
			static bool displayedUsingHours = false;

			uint8_t swHours = 0;
			uint8_t swMinutes = 0;
			uint8_t swSeconds = 0;
			stopwatch.getTime(&swHours, &swMinutes, &swSeconds);

			const bool usingHours = swHours > 0;
			const uint8_t displayHigh = usingHours ? swHours : swMinutes;
			const uint8_t displayLow = usingHours ? swMinutes : swSeconds;
			const bool timeChanged = (displayHigh != displayedHigh) || (displayLow != displayedLow);
			const bool modeChanged = usingHours != displayedUsingHours;

			if (!displayInitialised || timeChanged || modeChanged) {
				clockTextFormatter->prepareTimeString(displayHigh, displayLow, number_display_mode, clock_text, clock_colors);
				display->write_string(clock_text, clock_colors, true);
				displayedHigh = displayHigh;
				displayedLow = displayLow;
				displayedUsingHours = usingHours;
				displayInitialised = true;
			}
			break;
		}
		case CurrentMode::timer:
			if (!displayInitialised) {
				display->write_string("Timer", CRGB::Green, true);
				displayInitialised = true;
			}
			break;
		case CurrentMode::alarm:
			if (!displayInitialised) {
				display->write_string("Alarm", CRGB::Purple, true);
				displayInitialised = true;
			}
			break;
		case CurrentMode::settings:
			if (!displayInitialised) {
				display->write_string("Settings", CRGB::Red2, true);
				displayInitialised = true;
			}
			break;
	}

	// Update the display @ up to 60 frames per second
	display->tick();
	const uint32_t elapsed = millis() - startTime;
	const uint32_t frameDelay = (elapsed < TARGET_DELAY_BETWEEN_FRAMES)
		? (TARGET_DELAY_BETWEEN_FRAMES - elapsed)
		: 1;
	delay(frameDelay);
}

// Button Inputs
constexpr uint8_t kButtonPins[] = {2, 3, 4, 5, 6};
constexpr size_t kButtonCount = sizeof(kButtonPins) / sizeof(kButtonPins[0]);
constexpr uint8_t kDebounceLimit = 3;
constexpr uint32_t kDoubleClickThresholdMs = 300;
constexpr uint32_t kHoldThresholdMs = 800;

struct ButtonState {
	bool isPressed = false;
	uint8_t debounceCounter = 0;
	uint32_t pressStartMs = 0;
	bool holdEventSent = false;
	bool pendingSingleClick = false;
	uint32_t pendingSingleClickTime = 0;
};

void setup1() {
	r1->init();
	for (uint8_t pin : kButtonPins) {
		pinMode(pin, INPUT_PULLUP);
	}
}

// Polls for any button presses or rotary encoder movement
void loop1() {
	static ButtonState buttonStates[kButtonCount];

	bool rotaryInput[3];
	r1->loop(rotaryInput);

	if (rotaryInput[0]) {
		InputEventBuffer::push(InputKey::RotaryCW, InputEventType::Pressed);
	}
	if (rotaryInput[1]) {
		InputEventBuffer::push(InputKey::RotaryCCW, InputEventType::Pressed);
	}
	if (rotaryInput[2]) {
		InputEventBuffer::push(InputKey::RotaryButton, InputEventType::Pressed);
	}

	for (size_t i = 0; i < kButtonCount; ++i) {
		auto& state = buttonStates[i];
		const uint8_t pin = kButtonPins[i];
		const bool isPressedNow = digitalRead(pin) == LOW;
		const auto key = static_cast<InputKey>(
			static_cast<uint8_t>(InputKey::AuxButton0) + static_cast<uint8_t>(i));

		if (isPressedNow != state.isPressed) {
			state.debounceCounter++;
			if (state.debounceCounter >= kDebounceLimit) {
				state.isPressed = isPressedNow;
				state.debounceCounter = 0;

				if (isPressedNow) {
					state.pressStartMs = millis();
					state.holdEventSent = false;
				} else {
					if (!state.holdEventSent) {
						const uint32_t releaseTime = millis();
						if (state.pendingSingleClick &&
							(releaseTime - state.pendingSingleClickTime) <= kDoubleClickThresholdMs) {
							InputEventBuffer::push(key, InputEventType::DoubleClick, pin);
							state.pendingSingleClick = false;
						} else {
							state.pendingSingleClick = true;
							state.pendingSingleClickTime = releaseTime;
						}
					}
					state.holdEventSent = false;
				}
			}
		} else {
			state.debounceCounter = 0;
		}

		if (state.isPressed && !state.holdEventSent) {
			const uint32_t heldDuration = millis() - state.pressStartMs;
			if (heldDuration >= kHoldThresholdMs) {
				InputEventBuffer::push(key, InputEventType::Hold, pin);
				state.holdEventSent = true;
				state.pendingSingleClick = false;
			}
		}

		if (!state.isPressed && state.pendingSingleClick) {
			const uint32_t elapsed = millis() - state.pendingSingleClickTime;
			if (elapsed > kDoubleClickThresholdMs) {
				InputEventBuffer::push(key, InputEventType::Pressed, pin);
				state.pendingSingleClick = false;
			}
		}
	}

	delay(1);
}
