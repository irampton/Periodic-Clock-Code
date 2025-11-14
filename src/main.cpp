#include <Arduino.h>
#include <Wire.h>

#include "Display.h"
#include "DS3231_Wrapper.h"

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
Display* display = new Display(ROWS, COLUMNS, LED_PIN);

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
}

void loop() {
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
	InputEvent event{};
	while (InputEventBuffer::pop(event)) {
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
				if (event.type == InputEventType::Pressed) {
					switch (number_display_mode) {
						case NumberDisplayMode::Periodic:
							number_display_mode = NumberDisplayMode::Hour12;
							Serial.println("Clock mode: 12-hour");
							break;
						case NumberDisplayMode::Hour12:
							number_display_mode = NumberDisplayMode::Hour24;
							Serial.println("Clock mode: 24-hour");
							break;
						case NumberDisplayMode::Hour24:
							number_display_mode = NumberDisplayMode::Periodic;
							Serial.println("Clock mode: periodic");
							break;
					}
				} else {
					logButtonEvent("AuxButton0", event.type);
				}
				break;
			case InputKey::AuxButton1:
				logButtonEvent("AuxButton1", event.type);
				break;
			case InputKey::AuxButton2:
				logButtonEvent("AuxButton2", event.type);
				break;
			case InputKey::AuxButton3:
				logButtonEvent("AuxButton3", event.type);
				break;
			case InputKey::AuxButton4:
				logButtonEvent("AuxButton4", event.type);
				break;
		}
	}

	if (InputEventBuffer::consumeOverflowFlag()) {
		Serial.println("Input queue overflow");
	}

	switch (current_mode) {
		case CurrentMode::clock: {
			// Decide if we need to update the clock, then update it
			static bool displayInitialised = false;
			static int displayedHours = -1;
			static int displayedMinutes = -1;
			static NumberDisplayMode displayedMode = NumberDisplayMode::Periodic;

			const int hours = myRTC.getHours();
			const int minutes = myRTC.getMinutes();

			const bool timeChanged = (hours != displayedHours) || (minutes != displayedMinutes);
			const bool modeChanged = (number_display_mode != displayedMode);

			if (!displayInitialised || timeChanged || modeChanged) {
				std::string text;
				CRGB colors[CLOCK_LENGTH];

				getTime(hours, minutes, number_display_mode, text, colors);

				display->write_string(text, colors, !modeChanged);
				displayedHours = hours;
				displayedMinutes = minutes;
				displayedMode = number_display_mode;
				displayInitialised = true;
			}
			break;
		}
		case CurrentMode::stopwatch:
			break;
		case CurrentMode::timer:
			break;
		case CurrentMode::alarm:
			break;
		case CurrentMode::settings:
			break;
	}

	// Update the display @ up to 60 frames per second
	display->tick();
	delay(16);
}

// Button Inputs
constexpr uint8_t kButtonPins[] = {2, 3, 4, 5, 6};
constexpr size_t kButtonCount = sizeof(kButtonPins) / sizeof(kButtonPins[0]);
constexpr uint8_t kDebounceLimit = 3;
constexpr uint32_t kDoubleClickThresholdMs = 200;
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
