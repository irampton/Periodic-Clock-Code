#include "Carousel.h"

#include <algorithm>

Carousel::Carousel(CarouselItem* items, size_t itemCount, Display* display)
	: items_(items), itemCount_(itemCount), currentItemIndex_(0), display_(display) {}

CarouselItem& Carousel::currentItem() {
	return items_[currentItemIndex_ % itemCount_];
}

const CarouselItem& Carousel::currentItem() const {
	return items_[currentItemIndex_ % itemCount_];
}

void Carousel::applyCurrentOption(const CarouselItem& item) const {
	if (item.optionCount == 0 || item.currentIndex >= item.optionCount) {
		return;
	}
	if (item.options[item.currentIndex].apply) {
		item.options[item.currentIndex].apply();
	}
}

void Carousel::showCurrentOption(bool fade) const {
	const CarouselItem& item = currentItem();
	if (item.optionCount == 0 || item.currentIndex >= item.optionCount) {
		return;
	}
	const CarouselOption& option = item.options[item.currentIndex];
	display_->write_string(option.label, CRGB::Red2, fade);
}

void Carousel::nextItem() {
	if (itemCount_ == 0) {
		return;
	}
	currentItemIndex_ = (currentItemIndex_ + 1) % itemCount_;
	syncCurrentIndexFromProvider();
	applyCurrentOption(currentItem());
	showCurrentOption();
}

void Carousel::rotateOption(int direction) {
	CarouselItem& item = currentItem();
	if (item.optionCount == 0) {
		return;
	}

	const int count = static_cast<int>(item.optionCount);
	int index = static_cast<int>(item.currentIndex);
	index = (index + direction + count) % count;
	item.currentIndex = static_cast<size_t>(index);
	applyCurrentOption(item);
	showCurrentOption();
}

void Carousel::syncCurrentIndexFromProvider() {
	CarouselItem& item = currentItem();
	if (item.indexProvider && item.optionCount > 0) {
		item.currentIndex = item.indexProvider() % item.optionCount;
	}
}
