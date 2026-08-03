#include "InputEventBuffer.h"

bool InputEventBuffer::producerOverflowed = false;
bool InputEventBuffer::consumerOverflowed = false;

void InputEventBuffer::init() {
	// rp2040.fifo is initialized by the framework before either user core starts.
	consumerOverflowed = false;
}

bool InputEventBuffer::push(InputKey key, InputEventType type, uint8_t value) {
	// Preserve event ordering: report an earlier overflow before accepting new input.
	if (!flushOverflowMarker()) {
		return false;
	}

	if (!rp2040.fifo.push_nb(pack(key, type, value))) {
		producerOverflowed = true;
		return false;
	}
	return true;
}

bool InputEventBuffer::pop(InputEvent& event) {
	uint32_t packed = 0;
	while (rp2040.fifo.pop_nb(&packed)) {
		if (packed == kOverflowMarker) {
			consumerOverflowed = true;
			continue;
		}

		event = unpack(packed);
		return true;
	}

	return false;
}

bool InputEventBuffer::isEmpty() {
	return rp2040.fifo.available() == 0;
}

bool InputEventBuffer::consumeOverflowFlag() {
	if (!consumerOverflowed) {
		return false;
	}

	consumerOverflowed = false;
	return true;
}

void InputEventBuffer::service() {
	flushOverflowMarker();
}

uint32_t InputEventBuffer::pack(InputKey key, InputEventType type, uint8_t value) {
	return static_cast<uint32_t>(static_cast<uint8_t>(key)) |
	       (static_cast<uint32_t>(static_cast<uint8_t>(type)) << 8U) |
	       (static_cast<uint32_t>(value) << 16U);
}

InputEvent InputEventBuffer::unpack(uint32_t packed) {
	return {
		static_cast<InputKey>(packed & 0xFFu),
		static_cast<InputEventType>((packed >> 8U) & 0xFFu),
		static_cast<uint8_t>((packed >> 16U) & 0xFFu),
	};
}

bool InputEventBuffer::flushOverflowMarker() {
	if (!producerOverflowed) {
		return true;
	}
	if (!rp2040.fifo.push_nb(kOverflowMarker)) {
		return false;
	}
	producerOverflowed = false;
	return true;
}
