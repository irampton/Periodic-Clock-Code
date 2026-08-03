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
	// The RP2040 FIFO is initialized by the Arduino-Pico runtime before setup()/setup1().
	static void init();
	static bool push(InputKey key, InputEventType type, uint8_t value = 0);
	static bool pop(InputEvent& event);
	static bool isEmpty();
	static bool consumeOverflowFlag();
	// Call from the producer core to report a previously dropped event.
	static void service();

private:
	static constexpr uint32_t kOverflowMarker = 0xFFFFFFFFu;
	static bool producerOverflowed;
	static bool consumerOverflowed;

	static uint32_t pack(InputKey key, InputEventType type, uint8_t value);
	static InputEvent unpack(uint32_t packed);
	static bool flushOverflowMarker();
};

#endif // RP2040_INPUT_EVENT_BUFFER_H
