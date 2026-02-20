#!/usr/bin/env python3
"""Convert emoji PNG images to RLE-compressed RGB565 .bin files for ESP32 SPIFFS."""

import os
import sys
import struct
from pathlib import Path
from PIL import Image

IMG_SIZE = 72

def rgb888_to_rgb565(r, g, b):
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)

def rle_compress(pixels):
    """RLE compress a list of RGB565 uint16 values."""
    result = bytearray()
    i = 0
    n = len(pixels)
    while i < n:
        run_start = i
        while i < n - 1 and pixels[i] == pixels[i + 1] and (i - run_start) < 127:
            i += 1
        run_len = i - run_start + 1
        if run_len >= 3:
            result.append(0x80 | (run_len - 1))
            result.extend(struct.pack('>H', pixels[run_start]))
            i += 1
        else:
            i = run_start
            lit_start = i
            while i < n and (i - lit_start) < 127:
                if i < n - 2 and pixels[i] == pixels[i+1] == pixels[i+2]:
                    break
                i += 1
            lit_len = i - lit_start
            result.append(lit_len - 1)
            for j in range(lit_start, lit_start + lit_len):
                result.extend(struct.pack('>H', pixels[j]))
    return bytes(result)

def convert_image(input_path, output_path):
    img = Image.open(input_path).convert('RGB')
    img = img.resize((IMG_SIZE, IMG_SIZE), Image.LANCZOS)
    pixels = []
    for y in range(IMG_SIZE):
        for x in range(IMG_SIZE):
            r, g, b = img.getpixel((x, y))
            pixels.append(rgb888_to_rgb565(r, g, b))
    compressed = rle_compress(pixels)
    raw_size = IMG_SIZE * IMG_SIZE * 2
    with open(output_path, 'wb') as f:
        f.write(b'ORLE')
        f.write(struct.pack('>HHI', IMG_SIZE, IMG_SIZE, len(compressed)))
        f.write(compressed)
    return raw_size, len(compressed) + 12

def main():
    if len(sys.argv) < 3:
        print("Usage: convert_emoji.py <input_dir> <output_dir>")
        sys.exit(1)
    input_dir = Path(sys.argv[1])
    output_dir = Path(sys.argv[2])
    output_dir.mkdir(parents=True, exist_ok=True)
    total_raw = 0
    total_compressed = 0
    count = 0
    for png in sorted(input_dir.glob('*.png')):
        name = png.stem
        out_path = output_dir / f"{name}.bin"
        raw, compressed = convert_image(png, out_path)
        ratio = compressed / raw * 100
        print(f"  {name}: {raw} -> {compressed} bytes ({ratio:.1f}%)")
        total_raw += raw
        total_compressed += compressed
        count += 1
    print(f"\n{count} images converted")
    print(f"Total: {total_raw/1024:.1f}KB raw -> {total_compressed/1024:.1f}KB compressed")
    print(f"SPIFFS usage: {total_compressed/1024:.1f}KB of ~1500KB available")
    if total_compressed > 1500 * 1024:
        print("WARNING: Total exceeds SPIFFS capacity!")
        sys.exit(1)

if __name__ == '__main__':
    main()
