#include "Display.h"

#include <Arduino.h>

#define BRIGHTNESS_STEP 16
#define INITIAL_BRIGHTNESS 64
#define FADE_DURATION_MS 1000

Display::Display(int rows, int cols, int led_pin)
    : height(rows),
      width(cols),
      brightness(INITIAL_BRIGHTNESS),
      newData(false),
      fadeActive(false),
      fadeStartMillis(0),
      driver(led_pin, rows * cols) {
    const size_t totalPixels = static_cast<size_t>(height) * static_cast<size_t>(width);
    displayedFrame.assign(totalPixels, CRGB::Black);
    targetFrame = displayedFrame;
    fadeFromFrame = displayedFrame;
}

void Display::init() {
    driver.init();
    driver.setBrightness(brightness);
    driver.clearAll();
    newData = false;
    fadeActive = false;
    fadeStartMillis = 0;
}

void Display::write_string(char text[], CRGB* colors, bool fade) {
    if (text == nullptr) {
        return;
    }

    const size_t charCount = std::strlen(text);
    const int totalPixels = height * width;

    if (totalPixels <= 0) {
        return;
    }

    const size_t pixelCount = static_cast<size_t>(totalPixels);
    if (displayedFrame.size() != pixelCount) {
        displayedFrame.assign(pixelCount, CRGB::Black);
        targetFrame = displayedFrame;
        fadeFromFrame = displayedFrame;
    }

    std::fill(targetFrame.begin(), targetFrame.end(), CRGB::Black);

    if (charCount == 0) {
        // Let the fade logic handle transitions to a blank frame.
        fade = fade && (FADE_DURATION_MS > 0);
    }

    struct GlyphEntry {
        const Glyph* glyph;
        size_t charIndex;
    };

    std::vector<GlyphEntry> glyphs;
    glyphs.reserve(charCount);

    for (size_t i = 0; i < charCount; ++i) {
        const Glyph* glyph = font_lookup_ascii(text[i]);
        if (glyph != nullptr) {
            glyphs.push_back({glyph, i});
        }
    }

    if (glyphs.empty()) {
        // Nothing to draw beyond the cleared target frame.
        fade = fade && (FADE_DURATION_MS > 0);
    }

    std::vector<uint8_t> columns;
    std::vector<CRGB> columnColors;
    columns.reserve(width);
    columnColors.reserve(width);

    for (const GlyphEntry& entry : glyphs) {
        if (columns.size() >= static_cast<size_t>(width)) {
            break;
        }

        const Glyph& glyph = *entry.glyph;
        const size_t remaining = static_cast<size_t>(width) - columns.size();
        const uint8_t copyWidth = static_cast<uint8_t>(
            std::min(static_cast<size_t>(glyph.width), remaining));
        const CRGB glyphColor = colors ? colors[entry.charIndex] : CRGB::White;

        for (uint8_t col = 0; col < copyWidth; ++col) {
            const uint8_t columnByte = font_column_byte(glyph, col);
            columns.push_back(columnByte);
            columnColors.push_back(glyphColor);
        }
        if (columns.size() < width) {
            columns.push_back(0x00);
            columnColors.push_back(glyphColor);
        }
    }

    const int columnCount = std::min(columns.size(), static_cast<size_t>(width));
    const int totalSize = columnCount * height;

    for (int col = 0; col < columnCount; ++col) {
        const uint8_t columnBits = columns[col];
        const bool reverse = ((width - 1 - col) % 9) % 2 == 1;
        int colStart = totalSize - 1 - col * height;
        for (int row = 0; row < height; ++row) {
            const bool pixelOn = columnBits & (0b1 << row);
            const int ledIndex = reverse ? colStart - row : colStart - height + 1 + row;
            if (ledIndex >= 0 && ledIndex < totalPixels) {
                targetFrame[static_cast<size_t>(ledIndex)] = pixelOn ? columnColors[col] : CRGB::Black;
            }
        }
    }

    fade = fade && (FADE_DURATION_MS > 0);

    if (fade) {
        fadeFromFrame = displayedFrame;
        fadeActive = true;
        fadeStartMillis = millis();
        newData = false;
    } else {
        displayedFrame = targetFrame;
        newData = true;
        fadeActive = false;
    }
}

void Display::tick(){
    if (fadeActive) {
        const uint32_t elapsed = millis() - fadeStartMillis;
        const float duration = static_cast<float>(std::max(FADE_DURATION_MS, 1));
        float progress = static_cast<float>(elapsed) / duration;
        if (progress >= 1.0f) {
            progress = 1.0f;
        }
        renderFadeFrame(progress);
        driver.renderLEDs();
        if (progress >= 1.0f) {
            fadeActive = false;
            displayedFrame = targetFrame;
        }
    } else if (newData) {
        applyFrame(displayedFrame);
        driver.renderLEDs();
        newData = false;
    }
}

void Display::write_string(const std::string& text, CRGB* colors, bool fade) {
    // const_cast is fine here because your other function expects
    // a mutable char array, but we’re not modifying it.
    write_string(const_cast<char*>(text.c_str()), colors, fade);
}

void Display::incrementBrightness() {
    const uint16_t updated = static_cast<uint16_t>(brightness) + BRIGHTNESS_STEP;
    brightness = static_cast<uint8_t>(std::min<uint16_t>(updated, 255));
    driver.setBrightness(brightness);
}

void Display::decrementBrightness() {
    const int updated = static_cast<int>(brightness) - BRIGHTNESS_STEP;
    brightness = static_cast<uint8_t>(std::max(updated, 0));
    driver.setBrightness(brightness);
}

void Display::applyFrame(const std::vector<CRGB>& frame) {
    const size_t totalPixels = static_cast<size_t>(height) * static_cast<size_t>(width);
    const size_t copyCount = std::min(frame.size(), totalPixels);
    for (size_t idx = 0; idx < copyCount; ++idx) {
        driver.preSetLED(static_cast<int>(idx), frame[idx]);
    }
}

static CRGB lerpColor(const CRGB& c1, const CRGB& c2, float per) {
    const float clamped = std::clamp(per, 0.0f, 1.0f);
    const auto lerpComponent = [clamped](uint8_t start, uint8_t end) -> uint8_t {
        const float diff = static_cast<float>(end) - static_cast<float>(start);
        const float value = static_cast<float>(start) + diff * clamped;
        return static_cast<uint8_t>(std::clamp(value, 0.0f, 255.0f));
    };
    return CRGB(lerpComponent(c1.r, c2.r),
                lerpComponent(c1.g, c2.g),
                lerpComponent(c1.b, c2.b));
}

void Display::renderFadeFrame(float progress) {
    size_t totalPixels = displayedFrame.size();
    totalPixels = std::min(totalPixels, targetFrame.size());
    totalPixels = std::min(totalPixels, fadeFromFrame.size());
    for (size_t idx = 0; idx < totalPixels; ++idx) {
        const CRGB blended = lerpColor(fadeFromFrame[idx], targetFrame[idx], progress);
        displayedFrame[idx] = blended;
        driver.preSetLED(static_cast<int>(idx), blended);
    }
}
