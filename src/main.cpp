#include <Arduino.h>
#include <Wire.h>

#include "Rotary.h"
#include "DS3231_Wrapper.h"
#include "InputEventBuffer.h"
#include "Display.h"

#define ROWS 9
#define COLUMNS 7
#define LED_PIN 15

Rotary* r1 = new Rotary(9, 10, 8);
DS3231_Wrapper myRTC;
Display* display = new Display(ROWS, COLUMNS, LED_PIN);

void setup() {
    Wire.setSDA(0);
    Wire.setSCL(1);
    Wire.begin();

    InputEventBuffer::init();

    Serial.begin(9600);
}

char string[5];
CRGB colors[5];

void loop() {
    InputEvent event{};
    while (InputEventBuffer::pop(event)) {
        switch (event.type) {
        case InputEventType::RotaryCW:
            Serial.println("RotaryCW");
            break;
        case InputEventType::RotaryCCW:
            Serial.println("RotaryCCW");
            break;
        case InputEventType::RotaryButton:
            Serial.println("RotaryButton");
            break;
        case InputEventType::AuxButton0:
            myRTC.printTime();
            break;
        case InputEventType::AuxButton1:
            string[0] = 'a';
            string[1] = 'b';
            colors[0] = CRGB::Orange;
            colors[1] = colors[2] = colors[3] = colors[4] = CRGB::Green4;
            display->write_string(string, colors);
            Serial.println("AuxButton1");
            break;
        case InputEventType::AuxButton2:
            string[0] = 'Z';
            string[1] = 'A';
            colors[0] = CRGB::Red3;
            colors[1] = colors[2] = colors[3] = colors[4] = CRGB::Blue2;
            display->write_string(string, colors);
            Serial.println("AuxButton2");
            break;
        case InputEventType::AuxButton3:
            string[0] = 'Z';
            string[1] = 0;
            colors[0] = CRGB::Red3;
            colors[1] = colors[2] = colors[3] = colors[4] = CRGB::Blue2;
            display->write_string(string, colors);
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
constexpr int kBrightnessStep = 16;

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
