#include "Display.h"

#include <algorithm>
#include <cstring>
#include <vector>

Display::Display(int rows, int cols, int led_pin)
    : height(cols), width(rows), driver(led_pin, rows * cols) {
    driver.init();
}

void Display::write_string(char text[], CRGB* colors) {
    if (text == nullptr) {
        return;
    }

    const size_t charCount = std::strlen(text);
    const int totalPixels = height * width;

    if (totalPixels <= 0) {
        return;
    }

    for (int idx = 0; idx < totalPixels; ++idx) {
        driver.preSetLED(idx, CRGB::Black);
    }

    if (charCount == 0) {
        driver.renderLEDs();
        return;
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
        driver.renderLEDs();
        return;
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
    }

    const int columnCount = std::min(columns.size(), static_cast<size_t>(width));
    const int totalSize = columnCount * height;

    for (int col = 0; col < columnCount; ++col) {
        const uint8_t columnBits = columns[col];
        const bool reverse = (col % 2 == 1);
        int colStart = totalSize - 1 - col * height;
        for (int row = 0; row < height; ++row) {
            const bool pixelOn = columnBits & (0b1 << row);
            driver.preSetLED(reverse ? colStart - row : colStart - height + 1 + row,
                             pixelOn ? columnColors[col] : CRGB::Black);
        }
    }
    driver.renderLEDs();
}
