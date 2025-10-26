#!/usr/bin/env node
// build-font.js — packs column-major MSB-top, height forced to 7 rows.

const fs = require('fs');

const IN_PATH = './characters.json';
const OUT_PREFIX = 'font';
const HEIGHT = 7;

function die(msg) {
    console.error('Error:', msg);
    process.exit(1);
}

function hex8(v) {
    return '0x' + (v & 0xFF).toString(16).toUpperCase().padStart(2, '0');
}

function hex16(v) {
    return '0x' + (v & 0xFFFF).toString(16).toUpperCase().padStart(4, '0');
}

function readJsonArray(p) {
    const raw = fs.readFileSync(p, 'utf8');
    const data = JSON.parse(raw);
    if (!Array.isArray(data)) die('font.json must be an array of glyphs.');
    return data;
}

function findGlyphBitmap(pixelArr) {
    let bitmap
    if (pixelArr?.length > 0) {
        bitmap = (new Array(pixelArr[0].length)).fill(0);
        for (let i = 0; i < pixelArr[0].length; i++) {
            for (let j = 0; j < 8; j++) {
                bitmap[i] += (pixelArr?.[j]?.[i] || 0) << j;
            }
        }
    }
    return bitmap;
}

function buildFont() {
    const glyphs = readJsonArray(IN_PATH);

    // Sort by codepoint
    glyphs.sort((a, b) => a.codepoint - b.codepoint);

    // Pack & index
    const index = []; // {codepoint, offset, width}
    const bitmap = [];
    const asciiLut = new Array(256).fill(0xFFFF);
    let offset = 0;

    for (const g of glyphs) {
        const bits = findGlyphBitmap(g.pixels);
        if (!bits || !(bits.length > 0)) {
            continue;
        }
        index.push({codepoint: g.codepoint, offset, width: bits.length});
        bitmap.push(...bits);
        offset += bits.length;

        if (g.codepoint >= 0 && g.codepoint <= 256) {
            asciiLut[g.codepoint] = index.length - 1;
        }
    }

    writeCpp(HEIGHT, index, Uint8Array.from(bitmap), asciiLut);
    console.log(`Wrote ${OUT_PREFIX}.cpp`);
}

function writeCpp(height, index, bitmap, asciiLut) {
    const meta = `const FontMeta kFontMeta = { ${height}, ${index.length} };`;

    const idx = [
        `const Glyph kFontIndex[${index.length}] = {`,
        ...index.map(g => `  { ${hex16(g.codepoint)}, ${hex16(g.offset)}, ${g.width} },`),
        `};`
    ].join('\n');

    const lut = [
        `const uint16_t kAsciiToGlyphIdx[256] = {`,
        ...Array.from({length: 16}, (_, r) => {
            const base = r * 16;
            const row = asciiLut.slice(base, base + 16).map(hex16).join(', ');
            return `  ${row},`;
        }),
        `};`
    ].join('\n');

    const bmp = [
        `const uint8_t kFontBitmap[${bitmap.length}] = {`,
        ...(() => {
            const lines = [];
            for (let i = 0; i < bitmap.length; i += 16) {
                const chunk = [];
                for (let j = 0; j < 16 && i + j < bitmap.length; j++) chunk.push(hex8(bitmap[i + j]));
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

try {
    buildFont();
} catch (e) {
    console.error(e?.stack || e?.message || String(e));
    process.exit(1);
}
