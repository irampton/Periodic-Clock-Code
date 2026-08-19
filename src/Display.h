#pragma once

#include <string>
#include <algorithm>
#include <cstring>
#include <vector>
#include <cstdint>

#include "LED_Wrapper.h"
#include "font.h"

#define TARGET_DELAY_BETWEEN_FRAMES 12

class Display {
public:
	Display(int rows, int cols, int led_pin);
	void init();
	void write_characters(char text[], CRGB* colors, bool fade = false);
	void write_string(const std::string& text, CRGB* colors, bool fade = false);
	void write_string(const std::string& text, const CRGB& color, bool fade = false);
	// Draw a row-major, top-left-origin pixel buffer onto the physical LED matrix.
	void write_pixels(const CRGB* pixels, size_t count);
	void setBrightness(uint8_t value);
	uint8_t getBrightness() const;
	void incrementBrightness();
	void decrementBrightness();
	void strobe(bool enabled);
	void blink(bool left, bool center, bool right);
	void tick();

private:
	uint8_t height;
	uint8_t width;
	uint8_t brightness;
	bool newData;
	bool fadeActive;
	uint32_t fadeStartMillis;
	bool strobeActive;
	uint32_t strobeStartMillis;
	struct BlinkZoneState {
		bool active;
		uint32_t startMillis;
	};
	static constexpr int kBlinkZoneCount = 3;
	BlinkZoneState blinkZones[kBlinkZoneCount];
	LED_Wrapper driver;
	std::vector<CRGB> displayedFrame;
	std::vector<CRGB> targetFrame;
	std::vector<CRGB> fadeFromFrame;
	std::vector<int8_t> pixelZones;
	std::vector<uint8_t> scrollColumns;
	std::vector<CRGB> scrollColumnColors;
	std::vector<CRGB> scrollBlendFrame;
	float scrollPosition;
	bool scrollActive;
	uint32_t lastScrollUpdateMillis;
	uint32_t scrollPauseUntilMillis;
	void applyFrame(const std::vector<CRGB>& frame, float strobeMix, const float blinkMix[kBlinkZoneCount]);
	void renderFadeFrame(float progress, float strobeMix, const float blinkMix[kBlinkZoneCount]);

	struct GlyphColorEntry {
		const Glyph* glyph;
		CRGB color;
	};

	void ensureFrameSize();
	CRGB colorForIndex(size_t index, CRGB* colors, CRGB& lastColor, bool& hasColor) const;
	bool buildGlyphEntry(char character, size_t index, CRGB* colors, GlyphColorEntry& entry, CRGB& lastColor,
	                     bool& hasColor) const;
	std::vector<GlyphColorEntry> collectGlyphs(const char* text, size_t charCount, CRGB* colors) const;
	void buildColumns(const std::vector<GlyphColorEntry>& glyphs, size_t maxColumns, std::vector<uint8_t>& columns,
	                  std::vector<CRGB>& columnColors) const;
	void renderColumnsToFrame(const std::vector<uint8_t>& columns,
	                          const std::vector<CRGB>& columnColors,
	                          size_t startColumn,
	                          bool wrapColumns,
	                          std::vector<CRGB>& frame) const;
	void updateScrollTarget();
	bool advanceScrollPosition(float deltaSeconds);
	uint32_t scrollPauseDurationMs() const;
	void disableScroll();
	float strobeBlendAmount() const;
	CRGB applyStrobeToColor(const CRGB& baseColor, float strobeMix) const;
	bool isPixelLit(const CRGB& color) const;
	void updatePixelZones();
	void setBlinkZone(int zoneIndex, bool enabled);
	void populateBlinkMixes(float (&mixes)[kBlinkZoneCount]) const;
	float blinkBlendAmountForZone(int zoneIndex) const;
	bool anyBlinkZoneActive() const;
	CRGB applyBlinkToColor(const CRGB& baseColor, size_t pixelIndex, const float blinkMix[kBlinkZoneCount]) const;
};
