#ifndef RP2040_INPUT_EVENT_BUFFER_H
#define RP2040_INPUT_EVENT_BUFFER_H

#include <Arduino.h>

enum class InputKey : uint8_t {
    RotaryCW,
    RotaryCCW,
    RotaryButton,
    AuxButton0,
    AuxButton1,
    AuxButton2,
    AuxButton3,
    AuxButton4
};

enum class InputEventType : uint8_t {
    Pressed,
    DoubleClick,
    Hold
};

struct InputEvent {
    InputKey key;
    InputEventType type;
    uint8_t value;
};

class InputEventBuffer {
public:
    static void init();
    static bool push(InputKey key, InputEventType type, uint8_t value = 0);
    static bool pop(InputEvent& event);
    static bool isEmpty();
    static bool consumeOverflowFlag();

private:
    static constexpr uint8_t kCapacity = 32;
    static volatile uint8_t head;
    static volatile uint8_t tail;
    static volatile bool overflowed;
    static InputEvent buffer[kCapacity];
};

#endif // RP2040_INPUT_EVENT_BUFFER_H