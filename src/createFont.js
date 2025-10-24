#!/usr/bin/env node
// build-font.js — packs column-major MSB-top, height forced to 7 rows.

const fs = require('fs');

const IN_PATH = './characters.json';
const OUT_PREFIX = 'font';

function die(msg){ console.error('Error:', msg); process.exit(1); }
function hex8(v){ return '0x' + (v & 0xFF).toString(16).toUpperCase().padStart(2,'0'); }
function hex16(v){ return '0x' + (v & 0xFFFF).toString(16).toUpperCase().padStart(4,'0'); }

function readJsonArray(p){
    const raw = fs.readFileSync(p,'utf8');
    const data = JSON.parse(raw);
    if(!Array.isArray(data)) die('font.json must be an array of glyphs.');
    return data;
}

function normalizeTo7Rows(pixels){
    // pixels: [rows][cols] with 0/1 values; make exactly 7 rows, top-anchored.
    if(!Array.isArray(pixels) || pixels.length === 0) return null;
    const w = pixels[0].length;
    if(!Number.isInteger(w) || w <= 0) return null;
    // validate row widths; fill missing rows with zeros
    const out = [];
    for(let y=0; y<7; y++){
        if (y < pixels.length) {
            const row = pixels[y];
            if(!Array.isArray(row) || row.length !== w) return null;
            out.push(row.slice(0, w).map(v => v ? 1 : 0));
        } else {
            out.push(new Array(w).fill(0)); // pad at bottom
        }
    }
    return out;
}

function packColumnMajor7(pixels7){
    // 7 rows -> 1 byte per column, MSB = top row
    const h = 7;
    const w = pixels7[0].length;
    const out = new Uint8Array(w);
    for (let x=0; x<w; x++){
        let byte = 0;
        for (let y=0; y<h; y++){
            if (pixels7[y][x]) byte |= (0x80 >>> y); // MSB-top
        }
        out[x] = byte;
    }
    return out;
}

function buildFont(){
    const glyphsIn = readJsonArray(IN_PATH);

    // Filter BMP, normalize to 7 rows
    const glyphs = [];
    for (const g of glyphsIn){
        const cp = (g.codepoint != null) ? g.codepoint
            : (g.character && g.character.codePointAt ? g.character.codePointAt(0) : undefined);
        if (cp == null || !Number.isInteger(cp) || cp < 0) continue;
        if (cp > 0xFFFF) { console.warn(`Skipping non-BMP U+${cp.toString(16).toUpperCase()}`); continue; }
        const norm = normalizeTo7Rows(g.pixels);
        if (!norm) { console.warn(`Skipping codepoint U+${cp.toString(16).toUpperCase()} (bad pixels)`); continue; }
        glyphs.push({ codepoint: cp, pixels7: norm });
    }
    if (glyphs.length === 0) die('No valid glyphs after normalization.');

    // Sort by codepoint
    glyphs.sort((a,b)=>a.codepoint - b.codepoint);

    // Pack & index
    const HEIGHT = 7, COL_STRIDE = 1;
    const index = []; // {codepoint, offset, width}
    const bitmap = [];
    let offset = 0;

    for (const g of glyphs){
        const w = g.pixels7[0].length;
        const packed = packColumnMajor7(g.pixels7); // length = w
        index.push({ codepoint: g.codepoint, offset, width: w });
        for (let i=0;i<packed.length;i++) bitmap.push(packed[i]);
        offset += packed.length;
    }

    // ASCII LUT
    const asciiLut = new Array(256).fill(0xFFFF);
    for (let i=0;i<index.length;i++){
        const cp = index[i].codepoint;
        if (cp >= 0 && cp < 256) asciiLut[cp] = i;
    }

    writeHeader(HEIGHT, COL_STRIDE, index.length);
    writeCpp(HEIGHT, COL_STRIDE, index, Uint8Array.from(bitmap), asciiLut);
    console.log(`Wrote ${OUT_PREFIX}.h and ${OUT_PREFIX}.cpp`);
}

function writeHeader(height, colStride, glyphCount){
    const header = `#pragma once
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
  uint8_t  colStride;   // 1
  uint16_t glyphCount;
};

extern const FontMeta   kFontMeta;
extern const Glyph      kFontIndex[${glyphCount}];
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
  return &kFontBitmap[g.offset + x * kFontMeta.colStride];
}
inline uint8_t font_column_byte(const Glyph& g, int x){
  return kFontBitmap[g.offset + x];
}
`;
    fs.writeFileSync(`${OUT_PREFIX}.h`, header, 'utf8');
}

function writeCpp(height, colStride, index, bitmap, asciiLut){
    const meta = `const FontMeta kFontMeta = { ${height}, ${colStride}, ${index.length} };`;

    const idx = [
        `const Glyph kFontIndex[${index.length}] = {`,
        ...index.map(g => `  { ${hex16(g.codepoint)}, ${hex16(g.offset)}, ${g.width} },`),
        `};`
    ].join('\n');

    const lut = [
        `const uint16_t kAsciiToGlyphIdx[256] = {`,
        ...Array.from({length:16},(_,r)=> {
            const base=r*16;
            const row=asciiLut.slice(base, base+16).map(hex16).join(', ');
            return `  ${row},`;
        }),
        `};`
    ].join('\n');

    const bmp = [
        `const uint8_t kFontBitmap[${bitmap.length}] = {`,
        ...(() => {
            const lines=[];
            for (let i=0;i<bitmap.length;i+=16){
                const chunk=[];
                for(let j=0;j<16 && i+j<bitmap.length;j++) chunk.push(hex8(bitmap[i+j]));
                lines.push('  ' + chunk.join(', ') + ',');
            }
            return lines;
        })(),
        `};`
    ].join('\n');

    const cpp = `#include "${OUT_PREFIX}.h"
// clang-format off
${meta}

${idx}

${lut}

${bmp}
`;
    fs.writeFileSync(`${OUT_PREFIX}.cpp`, cpp, 'utf8');
}

try { buildFont(); }
catch(e){ console.error(e?.stack || e?.message || String(e)); process.exit(1); }
