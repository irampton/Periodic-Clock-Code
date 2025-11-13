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

ClockMode clock_mode = ClockMode::Periodic;

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
        switch (event.type) {
        case InputEventType::RotaryCW:
            display->incrementBrightness();
            Serial.println("RotaryCW");
            break;
        case InputEventType::RotaryCCW:
            display->decrementBrightness();
            Serial.println("RotaryCCW");
            break;
        case InputEventType::RotaryButton:
            Serial.println("RotaryButton");
            break;
        case InputEventType::AuxButton0:
            switch (clock_mode) {
            case ClockMode::Periodic:
                clock_mode = ClockMode::Hour12;
                Serial.println("Clock mode: 12-hour");
                break;
            case ClockMode::Hour12:
                clock_mode = ClockMode::Hour24;
                Serial.println("Clock mode: 24-hour");
                break;
            case ClockMode::Hour24:
                clock_mode = ClockMode::Periodic;
                Serial.println("Clock mode: periodic");
                break;
            }
            break;
        case InputEventType::AuxButton1:
            Serial.println("AuxButton1");
            break;
        case InputEventType::AuxButton2:
            Serial.println("AuxButton2");
            break;
        case InputEventType::AuxButton3:
            Serial.println("AuxButton3");
            break;
        case InputEventType::AuxButton4:
            Serial.println("AuxButton4");
            break;
        }
    }

    if (InputEventBuffer::consumeOverflowFlag()) {
        Serial.println("Input queue overflow");
    }

    // Decide if we need to update the clock, then update i
    static bool displayInitialised = false;
    static int displayedHours = -1;
    static int displayedMinutes = -1;
    static ClockMode displayedMode = ClockMode::Periodic;

    const int hours = myRTC.getHours();
    const int minutes = myRTC.getMinutes();

    const bool timeChanged = (hours != displayedHours) || (minutes != displayedMinutes);
    const bool modeChanged = (clock_mode != displayedMode);

    if (!displayInitialised || timeChanged || modeChanged) {
        std::string text;
        CRGB colors[CLOCK_LENGTH];

        getTime( hours, minutes, clock_mode, text, colors);

        display->write_string(text, colors, !modeChanged);
        displayedHours = hours;
        displayedMinutes = minutes;
        displayedMode = clock_mode;
        displayInitialised = true;
    }

	// Update the display @ up to 60 frames per second
	display->tick();
    delay(16);
}

// Button Inputs
constexpr uint8_t kButtonPins[] = {2, 3, 4, 5, 6};
constexpr size_t kButtonCount = sizeof(kButtonPins) / sizeof(kButtonPins[0]);
constexpr uint8_t kDebounceLimit = 3;

void setup1() {
    r1->init();
    for (uint8_t pin : kButtonPins) {
        pinMode(pin, INPUT_PULLUP);
    }
}

// Polls for any button presses or rotary encoder movement
void loop1() {
    static bool initialised = false;
    static bool buttonPressed[kButtonCount];
    static uint8_t debounceCounters[kButtonCount];

    if (!initialised) {
        for (size_t i = 0; i < kButtonCount; ++i) {
            buttonPressed[i] = false;
            debounceCounters[i] = 0;
        }
        initialised = true;
    }

    bool rotaryInput[3];
    r1->loop(rotaryInput);

    if (rotaryInput[0]) {
        InputEventBuffer::push(InputEventType::RotaryCW);
    }
    if (rotaryInput[1]) {
        InputEventBuffer::push(InputEventType::RotaryCCW);
    }
    if (rotaryInput[2]) {
        InputEventBuffer::push(InputEventType::RotaryButton);
    }

    for (size_t i = 0; i < kButtonCount; ++i) {
        const uint8_t pin = kButtonPins[i];
        const bool isPressed = digitalRead(pin) == LOW;

        if (isPressed != buttonPressed[i]) {
            debounceCounters[i]++;
            if (debounceCounters[i] >= kDebounceLimit) {
                buttonPressed[i] = isPressed;
                debounceCounters[i] = 0;

                if (isPressed) {
                    const auto type = static_cast<InputEventType>(
                        static_cast<uint8_t>(InputEventType::AuxButton0) + static_cast<uint8_t>(i));
                    InputEventBuffer::push(type, pin);
                }
            }
        } else {
            debounceCounters[i] = 0;
        }
    }

    delay(1);
}
