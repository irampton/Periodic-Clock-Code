#pragma once

#include <cstddef>

#include "Display.h"

struct CarouselOption {
	const char* label;
	void (*apply)();
};

struct CarouselItem {
	const char* label;
	CarouselOption* options;
	size_t optionCount;
	size_t currentIndex;
	// Optional function to fetch the current index from external state when entering this item.
	size_t (*indexProvider)();
};

// Generic carousel to cycle through settings and their options.
class Carousel {
public:
	Carousel(CarouselItem* items, size_t itemCount, Display* display);

	void nextItem();
	void rotateOption(int direction);
	void showCurrentOption(bool fade = true) const;
	void syncCurrentIndexFromProvider();
	void resetCarousel();

	CarouselItem& currentItem();
	const CarouselItem& currentItem() const;

private:
	CarouselItem* items_;
	size_t itemCount_;
	size_t currentItemIndex_;
	Display* display_;

	void applyCurrentOption(const CarouselItem& item) const;
};
