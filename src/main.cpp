#include <Arduino.h>
#include <Wire.h>
#include <cstdio>

#include "Rotary.h"
#include "DS3231_Wrapper.h"
#include "InputEventBuffer.h"
#include "Display.h"
#include "Periodic_Conversion.h"

#define ROWS 7
#define COLUMNS 27
#define LED_PIN 15

#define BUZZER_PIN 13

Rotary* r1 = new Rotary(9, 10, 8);
DS3231_Wrapper myRTC;
Display* display = new Display(ROWS, COLUMNS, LED_PIN);

enum class ClockMode {
    Hour12,
    Hour24,
    Periodic,
};

ClockMode clock_mode = ClockMode::Periodic;

bool isUnsignedInteger(const String& value) {
    if (value.length() == 0) {
        return false;
    }

    for (size_t i = 0; i < value.length(); ++i) {
        const char c = value.charAt(i);
        if (c < '0' || c > '9') {
            return false;
        }
    }

    return true;
}

void handleSerialCommand(const String& command) {
    String trimmed = command;
    trimmed.trim();
    if (trimmed.length() == 0) {
        return;
    }

    String lowered = trimmed;
    lowered.toLowerCase();

    if (!lowered.startsWith("set ")) {
        return;
    }

    String remainder = lowered.substring(4);
    remainder.trim();
    const int separator = remainder.indexOf(' ');
    if (separator < 0) {
        Serial.println("Invalid set command; expected format 'set <target> <value>'");
        return;
    }

    const String target = remainder.substring(0, separator);
    String valueToken = remainder.substring(separator + 1);
    valueToken.trim();

    if (!isUnsignedInteger(valueToken)) {
        Serial.println("Value must be numeric");
        return;
    }

    const int value = valueToken.toInt();

    if (target == "hour") {
        if (value >= 0 && value <= 23) {
            myRTC.setHours(static_cast<uint8_t>(value));
            Serial.print("Hour set to ");
            Serial.println(value);
        } else {
            Serial.println("Hour must be between 0 and 23");
        }
    } else if (target == "min" || target == "mins" || target == "minute" || target == "minutes") {
        if (value >= 0 && value <= 59) {
            myRTC.setMinutes(static_cast<uint8_t>(value));
            Serial.print("Minute set to ");
            Serial.println(value);
        } else {
            Serial.println("Minute must be between 0 and 59");
        }
    } else if (target == "sec" || target == "secs" || target == "second" || target == "seconds") {
        if (value >= 0 && value <= 59) {
            myRTC.setSeconds(static_cast<uint8_t>(value));
            Serial.print("Second set to ");
            Serial.println(value);
        } else {
            Serial.println("Second must be between 0 and 59");
        }
    } else {
        Serial.print("Unknown set target: ");
        Serial.println(target);
    }
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

std::string text;
CRGB colors[5];

void loop() {
    const int hours = myRTC.getHours();
    const int minutes = myRTC.getMinutes();

    switch (clock_mode) {
    case ClockMode::Hour12: {
        char buffer[6];
        int hour12 = hours % 12;
        if (hour12 == 0) {
            hour12 = 12;
        }
        std::snprintf(buffer, sizeof(buffer), "%2d:%02d", hour12, minutes);
        text.assign(buffer);
        for (CRGB& color : colors) {
            color = CRGB::Blue2;
        }
        display->write_string(text, colors);
        break;
    }
    case ClockMode::Hour24: {
        char buffer[6];
        std::snprintf(buffer, sizeof(buffer), "%02d:%02d", hours, minutes);
        text.assign(buffer);
        for (CRGB& color : colors) {
            color = CRGB::Blue2;
        }
        display->write_string(text, colors);
        break;
    }
    case ClockMode::Periodic:
        convert_to_periodic_time(hours, minutes, text, colors);
        display->write_string(text, colors);
        break;
    }

    static String serialBuffer;
    while (Serial.available() > 0) {
        const char incoming = static_cast<char>(Serial.read());
        if (incoming == '\r' || incoming == '\n') {
            if (serialBuffer.length() > 0) {
                handleSerialCommand(serialBuffer);
                serialBuffer = "";
            }
        } else if (serialBuffer.length() < 32) {
            serialBuffer += incoming;
        }
    }

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

    delay(1);
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
