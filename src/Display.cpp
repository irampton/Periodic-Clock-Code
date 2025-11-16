#include "Display.h"

#include <Arduino.h>
#include <limits>
#include <utility>
#include <cmath>

#define BRIGHTNESS_STEP 8
#define INITIAL_BRIGHTNESS 64
#define FADE_DURATION_MS 250
#define SCROLL_SPEED_PIXELS_PER_SECOND 35.0f
#define SCROLL_LOOP_DELAY_MULTIPLIER 15.0f
#define SCROLL_WRAP_SPACER_COLUMNS 27

Display::Display(int rows, int cols, int led_pin)
	: height(rows),
	  width(cols),
	  brightness(INITIAL_BRIGHTNESS),
	  newData(false),
	  fadeActive(false),
	  fadeStartMillis(0),
	  driver(led_pin, rows * cols),
	  scrollPosition(0.0f),
	  scrollActive(false),
	  lastScrollUpdateMillis(0),
	  scrollPauseUntilMillis(0) {
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
	disableScroll();
}

void Display::write_characters(char text[], CRGB* colors, bool fade) {
	if (text == nullptr) {
		return;
	}

	disableScroll();
	const size_t charCount = std::strlen(text);
	ensureFrameSize();
	if (targetFrame.empty()) {
		return;
	}

	std::vector<GlyphColorEntry> glyphs = collectGlyphs(text, charCount, colors);
	std::vector<uint8_t> columns;
	std::vector<CRGB> columnColors;
	columns.reserve(width);
	columnColors.reserve(width);
	buildColumns(glyphs, static_cast<size_t>(width), columns, columnColors);
	renderColumnsToFrame(columns, columnColors, 0, false, targetFrame);

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

void Display::tick() {
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
	} else if (scrollActive) {
		displayedFrame = targetFrame;
		applyFrame(displayedFrame);
		driver.renderLEDs();
		if (scrollColumns.empty()) {
			disableScroll();
		} else {
			const uint32_t now = millis();
			if (lastScrollUpdateMillis == 0) {
				lastScrollUpdateMillis = now;
			}
			if (scrollPauseUntilMillis != 0 && now < scrollPauseUntilMillis) {
				lastScrollUpdateMillis = now;
			} else {
				const float deltaSeconds = static_cast<float>(now - lastScrollUpdateMillis) / 1000.0f;
				if (deltaSeconds > 0.0f) {
					lastScrollUpdateMillis = now;
					const bool wrapped = advanceScrollPosition(deltaSeconds);
					updateScrollTarget();
					if (wrapped) {
						scrollPauseUntilMillis = now + scrollPauseDurationMs();
					} else {
						scrollPauseUntilMillis = 0;
					}
				}
			}
		}
	} else if (newData) {
		applyFrame(displayedFrame);
		driver.renderLEDs();
		newData = false;
	}
}

void Display::write_string(const std::string& text, CRGB* colors, bool fade) {
	disableScroll();
	const size_t charCount = text.size();

	if (charCount == 0) {
		write_characters(const_cast<char*>(""), colors, fade);
		return;
	}

	ensureFrameSize();
	if (targetFrame.empty()) {
		return;
	}

	std::vector<GlyphColorEntry> glyphs = collectGlyphs(text.c_str(), charCount, colors);
	std::vector<uint8_t> columns;
	std::vector<CRGB> columnColors;
	const size_t maxColumns = std::numeric_limits<size_t>::max();
	buildColumns(glyphs, maxColumns, columns, columnColors);

	if (columns.size() <= static_cast<size_t>(width)) {
		renderColumnsToFrame(columns, columnColors, 0, false, targetFrame);
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
		return;
	}

	scrollColumns = std::move(columns);
	scrollColumnColors = std::move(columnColors);
	for (int spacer = 0; spacer < SCROLL_WRAP_SPACER_COLUMNS; ++spacer) {
		scrollColumns.push_back(0x00);
		scrollColumnColors.push_back(CRGB::Black);
	}
	scrollPosition = 0.0f;
	scrollActive = true;
	const uint32_t now = millis();
	lastScrollUpdateMillis = now;
	scrollPauseUntilMillis = now + scrollPauseDurationMs();
	updateScrollTarget();

	fade = fade && (FADE_DURATION_MS > 0);
	if (fade) {
		fadeFromFrame = displayedFrame;
		fadeActive = true;
		fadeStartMillis = millis();
		newData = false;
	} else {
		displayedFrame = targetFrame;
		newData = false;
		fadeActive = false;
	}
}

void Display::write_string(const std::string& text, const CRGB& color, bool fade) {
	if (text.empty()) {
		write_string(text, static_cast<CRGB*>(nullptr), fade);
		return;
	}

	std::vector<CRGB> colorBuffer(text.size(), color);
	write_string(text, colorBuffer.data(), fade);
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
	const auto lerpComponent = [clamped](uint8_t start, uint8_t end) -> uint8_t
	{
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

void Display::ensureFrameSize() {
	const size_t totalPixels = static_cast<size_t>(height) * static_cast<size_t>(width);
	if (totalPixels == 0) {
		displayedFrame.clear();
		targetFrame.clear();
		fadeFromFrame.clear();
		return;
	}

	if (displayedFrame.size() != totalPixels) {
		displayedFrame.assign(totalPixels, CRGB::Black);
		targetFrame = displayedFrame;
		fadeFromFrame = displayedFrame;
	}
}

CRGB Display::colorForIndex(size_t index, CRGB* colors, CRGB& lastColor, bool& hasColor) const {
	if (colors == nullptr) {
		if (!hasColor) {
			lastColor = CRGB::White;
			hasColor = true;
		}
		return lastColor;
	}

	const CRGB candidate = colors[index];
	lastColor = candidate;
	hasColor = true;
	return candidate;
}

bool Display::buildGlyphEntry(char character,
                              size_t index,
                              CRGB* colors,
                              GlyphColorEntry& entry,
                              CRGB& lastColor,
                              bool& hasColor) const {
	const Glyph* glyph = font_lookup_ascii(character);
	if (glyph == nullptr) {
		return false;
	}

	entry.glyph = glyph;
	entry.color = colorForIndex(index, colors, lastColor, hasColor);
	return true;
}

std::vector<Display::GlyphColorEntry> Display::collectGlyphs(const char* text,
                                                             size_t charCount,
                                                             CRGB* colors) const {
	std::vector<GlyphColorEntry> glyphs;
	if (text == nullptr || charCount == 0) {
		return glyphs;
	}

	glyphs.reserve(charCount);
	CRGB lastColor = CRGB::White;
	bool hasColor = false;
	for (size_t idx = 0; idx < charCount; ++idx) {
		GlyphColorEntry entry{};
		if (buildGlyphEntry(text[idx], idx, colors, entry, lastColor, hasColor)) {
			glyphs.push_back(entry);
		}
	}
	return glyphs;
}

void Display::buildColumns(const std::vector<GlyphColorEntry>& glyphs,
                           size_t maxColumns,
                           std::vector<uint8_t>& columns,
                           std::vector<CRGB>& columnColors) const {
	columns.clear();
	columnColors.clear();

	if (glyphs.empty() || maxColumns == 0) {
		return;
	}

	const bool unlimited = (maxColumns == std::numeric_limits<size_t>::max());

	for (size_t idx = 0; idx < glyphs.size(); ++idx) {
		const Glyph& glyph = *glyphs[idx].glyph;
		const CRGB glyphColor = glyphs[idx].color;

		size_t remaining = unlimited ? glyph.width : (maxColumns > columns.size() ? maxColumns - columns.size() : 0);
		if (!unlimited && remaining == 0) {
			break;
		}

		const size_t copyWidth = unlimited ? glyph.width : std::min(remaining, static_cast<size_t>(glyph.width));
		for (size_t col = 0; col < copyWidth; ++col) {
			const uint8_t columnByte = font_column_byte(glyph, static_cast<int>(col));
			columns.push_back(columnByte);
			columnColors.push_back(glyphColor);
		}

		const bool hasMoreGlyphs = (idx + 1) < glyphs.size();
		if (hasMoreGlyphs && (unlimited || columns.size() < maxColumns)) {
			columns.push_back(0x00);
			columnColors.push_back(glyphColor);
		}
	}
}

void Display::renderColumnsToFrame(const std::vector<uint8_t>& columns,
                                   const std::vector<CRGB>& columnColors,
                                   size_t startColumn,
                                   bool wrapColumns,
                                   std::vector<CRGB>& frame) const {
	const size_t totalPixels = static_cast<size_t>(height) * static_cast<size_t>(width);
	if (frame.size() != totalPixels) {
		return;
	}

	std::fill(frame.begin(), frame.end(), CRGB::Black);

	if (height == 0 || width == 0 || columns.empty()) {
		return;
	}

	size_t effectiveColumns = static_cast<size_t>(width);
	if (!wrapColumns) {
		if (startColumn >= columns.size()) {
			return;
		}
		const size_t remaining = columns.size() - startColumn;
		effectiveColumns = std::min(remaining, static_cast<size_t>(width));
	}

	const int columnCount = static_cast<int>(effectiveColumns);
	if (columnCount <= 0) {
		return;
	}

	const int totalSize = columnCount * height;
	for (int col = 0; col < columnCount; ++col) {
		size_t sourceIdx = wrapColumns
			                   ? (startColumn + static_cast<size_t>(col)) % columns.size()
			                   : startColumn + static_cast<size_t>(col);
		if (!wrapColumns && sourceIdx >= columns.size()) {
			break;
		}

		const uint8_t columnBits = columns[sourceIdx];
		const CRGB columnColor = columnColors[sourceIdx];
		const bool reverse = ((width - 1 - col) % 9) % 2 == 1;
		const int colStart = totalSize - 1 - col * height;
		for (int row = 0; row < height; ++row) {
			const bool pixelOn = (columnBits & (0b1 << row)) != 0;
			const int ledIndex = reverse ? colStart - row : colStart - height + 1 + row;
			if (ledIndex >= 0 && ledIndex < static_cast<int>(totalPixels)) {
				frame[static_cast<size_t>(ledIndex)] = pixelOn ? columnColor : CRGB::Black;
			}
		}
	}
}

void Display::updateScrollTarget() {
	if (!scrollActive || scrollColumns.empty()) {
		return;
	}

	const size_t columnCount = scrollColumns.size();
	const float integralPart = std::floor(scrollPosition);
	const size_t baseColumn = columnCount == 0 ? 0 : static_cast<size_t>(integralPart) % columnCount;
	renderColumnsToFrame(scrollColumns, scrollColumnColors, baseColumn, true, targetFrame);

	const float fractional = std::clamp(scrollPosition - integralPart, 0.0f, 1.0f);
	if (fractional <= 0.0f || columnCount == 0) {
		return;
	}

	if (scrollBlendFrame.size() != targetFrame.size()) {
		scrollBlendFrame.assign(targetFrame.size(), CRGB::Black);
	}

	const size_t nextColumn = (baseColumn + 1) % columnCount;
	renderColumnsToFrame(scrollColumns, scrollColumnColors, nextColumn, true, scrollBlendFrame);

#ifdef FADE_BETWEEN_FRAMES
	const size_t totalPixels = targetFrame.size();
	for (size_t idx = 0; idx < totalPixels; ++idx) {
		targetFrame[idx] = lerpColor(targetFrame[idx], scrollBlendFrame[idx], fractional);
	}
#endif
}

bool Display::advanceScrollPosition(float deltaSeconds) {
	if (scrollColumns.empty() || deltaSeconds <= 0.0f) {
		return false;
	}

	const float speed = std::max(SCROLL_SPEED_PIXELS_PER_SECOND, 1.0f);
	scrollPosition += speed * deltaSeconds;
	const float totalColumns = static_cast<float>(scrollColumns.size());

	if (totalColumns <= 0.0f) {
		scrollPosition = 0.0f;
		return false;
	}

	if (scrollPosition >= totalColumns) {
		scrollPosition = 0.0f;
		return true;
	}
	return false;
}

uint32_t Display::scrollPauseDurationMs() const {
	const float speed = std::max(SCROLL_SPEED_PIXELS_PER_SECOND, 1.0f);
	const float pixelDurationMs = 1000.0f / speed;
	const float requestedDelay = pixelDurationMs * SCROLL_LOOP_DELAY_MULTIPLIER;
	const float clampedDelay = std::max(requestedDelay, 1.0f);
	return static_cast<uint32_t>(clampedDelay);
}

void Display::disableScroll() {
	scrollActive = false;
	scrollPosition = 0.0f;
	scrollColumns.clear();
	scrollColumnColors.clear();
	scrollBlendFrame.clear();
	lastScrollUpdateMillis = 0;
	scrollPauseUntilMillis = 0;
}
