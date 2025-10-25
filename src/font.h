#pragma once
#include <cstdint>

// === Generated font (column-major, MSB-top), height forced to 7 ===
// Each column: 1 byte (bit7=top row ... bit1=row6, bit0 unused)

struct Glyph {
  uint16_t codepoint;   // Unicode BMP
  uint16_t offset;      // into kFontBitmap[]
  uint8_t  width;       // columns
};

struct FontMeta {
  uint8_t  height;      // 7
  uint16_t glyphCount;
};

extern const FontMeta   kFontMeta;
extern const Glyph      kFontIndex[1186];
extern const uint8_t    kFontBitmap[];
extern const uint16_t   kAsciiToGlyphIdx[256];

inline const Glyph* font_lookup_u16(uint16_t cp){
  int lo=0, hi=(int)kFontMeta.glyphCount-1;
  while(lo<=hi){
    int mid=(lo+hi)>>1;
    uint16_t c=kFontIndex[mid].codepoint;
    if(c==cp) return &kFontIndex[mid];
    if(c<cp) lo=mid+1; else hi=mid-1;
  }
  return nullptr;
}
inline const Glyph* font_lookup_ascii(char c){
  uint8_t idx=(uint8_t)c;
  uint16_t gi=kAsciiToGlyphIdx[idx];
  if(gi!=0xFFFF) return &kFontIndex[gi];
  return font_lookup_u16(idx);
}
inline const uint8_t* font_column_bytes(const Glyph& g, int x){
  return &kFontBitmap[g.offset + x];
}
inline uint8_t font_column_byte(const Glyph& g, int x){
  return kFontBitmap[g.offset + x];
}
