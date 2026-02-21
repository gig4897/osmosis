#!/usr/bin/env python3
"""Render emoji using Apple Color Emoji font on macOS.

Produces 96x96 PNG images with black background for use with the
Osmosis firmware (where RGB565 black = 0x0000 = transparent).

Usage:
    pip install pyobjc pyobjc-framework-Cocoa
    python render_apple_emoji.py <output_dir> [--csv <vocab_csv_dir>]

Reads emoji codepoints from vocab CSV files in the specified directory,
or falls back to parsing word_data.h for backward compatibility.
"""

import os
import sys
import re
import csv
from pathlib import Path

try:
    import AppKit
    from AppKit import (
        NSFont, NSAttributedString, NSForegroundColorAttributeName,
        NSFontAttributeName, NSGraphicsContext, NSCompositingOperationSourceOver,
        NSImage, NSBitmapImageRep, NSPNGFileType, NSCalibratedRGBColorSpace,
        NSMakeRect, NSColor
    )
    from Foundation import NSString, NSDictionary, NSMakeSize
except ImportError:
    print("ERROR: pyobjc is required. Install with:")
    print("  pip install pyobjc pyobjc-framework-Cocoa")
    sys.exit(1)

IMG_SIZE = 96
FONT_SIZE = 86  # Slightly smaller than canvas to avoid clipping


def parse_vocab_csvs(vocab_dir):
    """Parse all vocab CSV files to extract unique emoji codepoints."""
    codepoints = set()
    vocab_path = Path(vocab_dir)
    for csv_file in sorted(vocab_path.glob('*.csv')):
        with open(csv_file) as f:
            reader = csv.DictReader(f)
            for row in reader:
                cp = row.get('emoji', '').strip()
                if cp:
                    codepoints.add(cp)
    return sorted(codepoints)


def parse_word_data(src_dir):
    """Parse word_data.h to extract imageFile -> emoji mappings (legacy)."""
    word_data_path = src_dir / "src" / "word_data.h"
    if not word_data_path.exists():
        return []
    content = word_data_path.read_text()

    entries = []
    pattern = r'\{"[^"]*",\s*"[^"]*",\s*"([^"]*)",\s*"[^"]*",\s*"[^"]*"\}.*?//.*?(U\+[0-9A-Fa-f]+(?:\+[0-9A-Fa-f]+)*)'

    for match in re.finditer(pattern, content):
        image_file = match.group(1)
        codepoints_str = match.group(2)

        codepoints = []
        for cp in codepoints_str.split('+'):
            cp = cp.strip()
            if cp.startswith('U'):
                cp = cp[1:]
            try:
                codepoints.append(int(cp, 16))
            except ValueError:
                continue

        if codepoints:
            emoji_char = ''.join(chr(cp) for cp in codepoints)
            # Use codepoint hex as filename (e.g. "1f4a7")
            cp_hex = codepoints_str.replace('U+', '').replace('+', '_').lower()
            entries.append((cp_hex, emoji_char, codepoints_str))

    return entries


def codepoint_to_emoji(cp_hex):
    """Convert a codepoint hex string like '1f4a7' or '1f468_1f692' to emoji chars."""
    parts = cp_hex.split('_')
    chars = []
    for part in parts:
        try:
            chars.append(chr(int(part, 16)))
        except ValueError:
            pass
    return ''.join(chars)


def render_emoji_to_png(emoji_char, output_path, size=IMG_SIZE):
    """Render a single emoji character to a PNG file using Apple Color Emoji."""
    image = NSImage.alloc().initWithSize_(NSMakeSize(size, size))
    image.lockFocus()

    NSColor.blackColor().setFill()
    AppKit.NSRectFill(NSMakeRect(0, 0, size, size))

    font = NSFont.fontWithName_size_("Apple Color Emoji", FONT_SIZE)
    if font is None:
        font = NSFont.systemFontOfSize_(FONT_SIZE)

    attrs = {
        NSFontAttributeName: font,
    }
    attr_str = NSAttributedString.alloc().initWithString_attributes_(emoji_char, attrs)

    text_size = attr_str.size()
    x = (size - text_size.width) / 2
    y = (size - text_size.height) / 2

    attr_str.drawAtPoint_((x, y))
    image.unlockFocus()

    tiff_data = image.TIFFRepresentation()
    bitmap_rep = NSBitmapImageRep.imageRepWithData_(tiff_data)
    png_data = bitmap_rep.representationUsingType_properties_(NSPNGFileType, {})
    png_data.writeToFile_atomically_(str(output_path), True)
    return True


def main():
    if len(sys.argv) < 2:
        print("Usage: render_apple_emoji.py <output_dir> [--csv <vocab_csv_dir>]")
        sys.exit(1)

    output_dir = Path(sys.argv[1])
    output_dir.mkdir(parents=True, exist_ok=True)

    script_dir = Path(__file__).resolve().parent
    project_root = script_dir.parent

    # Check for --csv argument
    csv_dir = None
    if '--csv' in sys.argv:
        idx = sys.argv.index('--csv')
        if idx + 1 < len(sys.argv):
            csv_dir = sys.argv[idx + 1]

    entries = []
    if csv_dir:
        # Read codepoints from vocab CSVs
        codepoints = parse_vocab_csvs(csv_dir)
        print(f"Found {len(codepoints)} unique emoji codepoints from CSVs")
        for cp in codepoints:
            emoji_char = codepoint_to_emoji(cp)
            entries.append((cp, emoji_char, f"U+{cp.upper()}"))
    else:
        # Legacy: parse word_data.h
        entries = parse_word_data(project_root)
        print(f"Found {len(entries)} emoji entries in word_data.h")

    success = 0
    failed = 0

    for filename, emoji_char, codepoint_str in entries:
        output_path = output_dir / f"{filename}.png"
        try:
            render_emoji_to_png(emoji_char, output_path)
            print(f"  OK {filename} ({codepoint_str})")
            success += 1
        except Exception as e:
            print(f"  FAIL {filename} ({codepoint_str}): {e}")
            failed += 1

    print(f"\nDone: {success} rendered, {failed} failed")


if __name__ == '__main__':
    main()
