#include "InputEventBuffer.h"

volatile uint8_t InputEventBuffer::head = 0;
volatile uint8_t InputEventBuffer::tail = 0;
volatile bool InputEventBuffer::overflowed = false;
InputEvent InputEventBuffer::buffer[InputEventBuffer::kCapacity];

void InputEventBuffer::init() {
    head = 0;
    tail = 0;
    overflowed = false;
}

bool InputEventBuffer::push(InputEventType type, uint8_t value) {
    const uint8_t nextHead = static_cast<uint8_t>((head + 1U) % kCapacity);
    if (nextHead == tail) {
        overflowed = true;
        return false;
    }

    buffer[head] = {type, value};
    head = nextHead;
    return true;
}

bool InputEventBuffer::pop(InputEvent& event) {
    if (head == tail) {
        return false;
    }

    event = buffer[tail];
    tail = static_cast<uint8_t>((tail + 1U) % kCapacity);
    return true;
}

bool InputEventBuffer::isEmpty() {
    return head == tail;
}

bool InputEventBuffer::consumeOverflowFlag() {
    if (!overflowed) {
        return false;
    }

    overflowed = false;
    return true;
}
