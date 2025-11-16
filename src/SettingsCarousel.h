#pragma once

#include "Carousel.h"

class ClockText;
class Display;

// Initializes the settings carousel. Must be called before accessing the carousel.
void initSettingsCarousel(Display* display, ClockText* clockText);

// Accessor for the shared settings carousel instance.
Carousel& getSettingsCarousel();
