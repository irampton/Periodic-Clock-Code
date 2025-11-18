#pragma once

#include "Carousel.h"

class ClockText;
class Display;
class DS3231_Wrapper;
class PersistentSettings;

// Initializes the settings carousel. Must be called before accessing the carousel.
void initSettingsCarousel(Display* display, ClockText* clockText, PersistentSettings* persistentSettings, DS3231_Wrapper* rtcWrapper);

// Accessor for the shared settings carousel instance.
Carousel& getSettingsCarousel();
