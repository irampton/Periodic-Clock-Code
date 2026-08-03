#include "SnakeGame.h"

#include <algorithm>
#include <cmath>

#include "Display.h"

static constexpr float kTwoPi = 6.28318530718f;

void SnakeGame::init(Display* display, uint8_t width, uint8_t height) {
	display_ = display;
	width_ = width;
	height_ = height;
	const size_t pixelCount = static_cast<size_t>(width_) * height_;
	snake_.reserve(pixelCount);
	frame_.assign(pixelCount, CRGB::Black);
}

void SnakeGame::start() {
	if (display_ == nullptr || width_ == 0 || height_ == 0) {
		return;
	}

	active_ = true;
	gameOver_ = false;
	foodVisible_ = false;
	direction_ = Direction::Right;
	pendingTurn_ = 0;
	snake_.clear();
	snake_.push_back({static_cast<int8_t>(width_ / 2), static_cast<int8_t>(height_ / 2)});
	randomSeed(micros());
	spawnFood();
	lastMoveMs_ = millis();
	render();
}

void SnakeGame::stop() {
	active_ = false;
}

void SnakeGame::turnRight() {
	if (active_ && !gameOver_) {
		pendingTurn_ = 1;
	}
}

void SnakeGame::turnLeft() {
	if (active_ && !gameOver_) {
		pendingTurn_ = -1;
	}
}

void SnakeGame::tick() {
	if (!active_) {
		return;
	}

	const uint32_t now = millis();
	if (gameOver_) {
		render();
		return;
	}

	if (now - lastMoveMs_ >= kMoveIntervalMs) {
		lastMoveMs_ = now;
		move();
		render();
	}
}

void SnakeGame::move() {
	if (pendingTurn_ != 0) {
		const uint8_t offset = pendingTurn_ > 0 ? 1 : 3;
		direction_ = static_cast<Direction>((static_cast<uint8_t>(direction_) + offset) % 4);
		pendingTurn_ = 0;
	}

	Point next = snake_.front();
	switch (direction_) {
		case Direction::Up:    --next.y; break;
		case Direction::Right: ++next.x; break;
		case Direction::Down:  ++next.y; break;
		case Direction::Left:  --next.x; break;
	}

	if (next.x < 0) next.x = static_cast<int8_t>(width_ - 1);
	if (next.x >= static_cast<int8_t>(width_)) next.x = 0;
	if (next.y < 0) next.y = static_cast<int8_t>(height_ - 1);
	if (next.y >= static_cast<int8_t>(height_)) next.y = 0;

	const bool ateFood = foodVisible_ && next == food_;
	const size_t collisionCount = ateFood ? snake_.size() : snake_.size() - 1;
	if (contains(next, collisionCount)) {
		gameOver_ = true;
		gameOverMs_ = millis();
		return;
	}

	snake_.insert(snake_.begin(), next);
	if (ateFood) {
		if (!spawnFood()) {
			gameOver_ = true;
			gameOverMs_ = millis();
		}
	} else {
		snake_.pop_back();
	}
}

bool SnakeGame::contains(const Point& point, size_t count) const {
	const size_t end = std::min(count, snake_.size());
	for (size_t i = 0; i < end; ++i) {
		if (snake_[i] == point) {
			return true;
		}
	}
	return false;
}

bool SnakeGame::spawnFood() {
	const size_t totalPixels = static_cast<size_t>(width_) * height_;
	if (snake_.size() >= totalPixels) {
		foodVisible_ = false;
		return false;
	}

	const size_t freeIndex = static_cast<size_t>(random(static_cast<long>(totalPixels - snake_.size())));
	size_t seenFree = 0;
	for (size_t i = 0; i < totalPixels; ++i) {
		const Point candidate{static_cast<int8_t>(i % width_), static_cast<int8_t>(i / width_)};
		if (contains(candidate, snake_.size())) {
			continue;
		}
		if (seenFree == freeIndex) {
			food_ = candidate;
			foodVisible_ = true;
			return true;
		}
		++seenFree;
	}

	foodVisible_ = false;
	return false;
}

void SnakeGame::render() {
	std::fill(frame_.begin(), frame_.end(), CRGB::Black);
	uint8_t green = 255;
	if (gameOver_) {
		const uint32_t phaseMs = (millis() - gameOverMs_) % kBreathingPeriodMs;
		const float phase = static_cast<float>(phaseMs) / static_cast<float>(kBreathingPeriodMs);
		const float wave = 0.5f * (1.0f - std::cos(phase * kTwoPi));
		green = static_cast<uint8_t>(kMinimumBreathingBrightness +
			(255 - kMinimumBreathingBrightness) * wave);
	}

	for (const Point& point : snake_) {
		frame_[static_cast<size_t>(point.y) * width_ + point.x] = CRGB(0, green, 0);
	}
	if (!snake_.empty()) {
		const Point& head = snake_.front();
		frame_[static_cast<size_t>(head.y) * width_ + head.x] = CRGB(0, green / 2, 0);
	}
	if (foodVisible_) {
		frame_[static_cast<size_t>(food_.y) * width_ + food_.x] = CRGB::Red;
	}
	display_->write_pixels(frame_.data(), frame_.size());
}
