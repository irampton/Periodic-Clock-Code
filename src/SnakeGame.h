#pragma once

#include <Arduino.h>
#include <FastLED.h>
#include <vector>

class Display;

class SnakeGame {
public:
	void init(Display* display, uint8_t width, uint8_t height);
	void start();
	void stop();
	void turnRight();
	void turnLeft();
	void tick();

private:
	struct Point {
		int8_t x;
		int8_t y;

		bool operator==(const Point& other) const {
			return x == other.x && y == other.y;
		}
	};

	enum class Direction : uint8_t { Up, Right, Down, Left };

	static constexpr uint32_t kMoveIntervalMs = 1000;
	static constexpr uint32_t kBreathingPeriodMs = 2500;
	static constexpr uint8_t kMinimumBreathingBrightness = 26;

	Display* display_ = nullptr;
	uint8_t width_ = 0;
	uint8_t height_ = 0;
	std::vector<Point> snake_;
	std::vector<CRGB> frame_;
	Point food_{0, 0};
	Direction direction_ = Direction::Right;
	int8_t pendingTurn_ = 0;
	bool active_ = false;
	bool gameOver_ = false;
	bool foodVisible_ = false;
	uint32_t lastMoveMs_ = 0;
	uint32_t gameOverMs_ = 0;

	void move();
	bool contains(const Point& point, size_t count) const;
	bool spawnFood();
	void render();
};
