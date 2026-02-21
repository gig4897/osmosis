#!/usr/bin/env python3
"""
Generate .vlw smooth font files for TFT_eSPI from system TrueType fonts.
Supports multiple languages with per-language special character sets.

Usage:
    python3 tools/generate_vlw_font.py --language spanish --size 26 --output data/font.vlw
"""

import argparse
import struct
import sys
from pathlib import Path

try:
    from PIL import Image, ImageDraw, ImageFont
except ImportError:
    print("Error: Pillow not installed. Run: pip3 install Pillow")
    sys.exit(1)


# Per-language special characters (added to ASCII printable set)
# For non-Latin scripts, we extract unique chars from the vocab CSVs at build time.
LANGUAGE_CHARS = {
    'spanish': list("áéíóúñüÁÉÍÓÚÑÜ¿¡"),
    'french': list("àâæçéèêëîïôœùûüÿÀÂÆÇÉÈÊËÎÏÔŒÙÛÜŸ"),
    'portuguese': list("àáâãçéêíóôõúÀÁÂÃÇÉÊÍÓÔÕÚ"),
    'dutch': list("éëïüÉËÏÜ"),
    'chinese': list("āáǎàēéěèīíǐìōóǒòūúǔùǖǘǚǜĀÁǍÀĒÉĚÈĪÍǏÌŌÓǑÒŪÚǓÙǕǗǙǛ"),
    'hindi': "auto",      # Extract Devanagari chars from CSVs
    'arabic': "auto",     # Extract Arabic script chars from CSVs
    'urdu': "auto",       # Extract Urdu/Arabic script chars from CSVs
    'japanese': "auto",   # Extract kanji/kana chars from CSVs
    'korean': "auto",     # Extract Hangul chars from CSVs
    'qeqchi': list("'"),  # Q'eqchi' uses Latin + glottal stop apostrophe
    'tsalagi': "auto",    # Extract Cherokee syllabary from CSVs
    'mvskoke': list(""),  # Mvskoke uses standard Latin (v is already in ASCII)
}


def extract_chars_from_csv(language):
    """Extract unique non-ASCII characters from a language's vocab CSVs."""
    import csv
    vocab_dir = Path(__file__).parent / "vocab"
    chars = set()
    for tier in ['beginner', 'intermediate', 'advanced', 'expert']:
        csv_path = vocab_dir / f"{language}_{tier}.csv"
        if not csv_path.exists():
            continue
        with open(csv_path, encoding='utf-8') as f:
            reader = csv.DictReader(f)
            for row in reader:
                for c in row.get('translation', ''):
                    if ord(c) > 0x7E:
                        chars.add(c)
    return sorted(chars, key=ord)

# ASCII printable characters (0x21 - 0x7E)
ASCII_CHARS = [chr(c) for c in range(0x21, 0x7F)]


def get_char_set(language):
    """Get the full character set for a language."""
    extra = LANGUAGE_CHARS.get(language, [])
    if extra == "auto":
        extra = extract_chars_from_csv(language)
        if not extra:
            print(f"  Warning: No non-ASCII chars found in CSVs for {language}")
            extra = []
    # Deduplicate while preserving order
    seen = set()
    chars = []
    for c in ASCII_CHARS + extra:
        if c not in seen:
            seen.add(c)
            chars.append(c)
    return chars


def get_system_font(size, language=None):
    """Try to find a good system font for the given size and language.

    For non-Latin scripts, we need specific fonts that contain those glyphs.
    On macOS, the system provides fonts for most scripts.
    """
    # Script-specific fonts (macOS paths)
    SCRIPT_FONTS = {
        'tsalagi': [
            "/System/Library/Fonts/AppleSDGothicNeo.ttc",
            "/System/Library/Fonts/Supplemental/Plantagenet Cherokee.ttf",
        ],
        'korean': [
            "/System/Library/Fonts/AppleSDGothicNeo.ttc",
            "/System/Library/Fonts/Supplemental/AppleMyungjo.ttf",
            "/Library/Fonts/NanumGothic.ttc",
        ],
        'japanese': [
            "/System/Library/Fonts/ヒラギノ角ゴシック W3.ttc",
            "/System/Library/Fonts/Hiragino Sans GB.ttc",
            "/System/Library/Fonts/Supplemental/Hiragino Sans W3.ttc",
            "/Library/Fonts/Yu Gothic.ttf",
        ],
        'arabic': [
            "/System/Library/Fonts/GeezaPro.ttc",
            "/Library/Fonts/GeezaPro.ttf",
        ],
        'urdu': [
            "/System/Library/Fonts/GeezaPro.ttc",
            "/Library/Fonts/GeezaPro.ttf",
        ],
        'hindi': [
            "/System/Library/Fonts/Kohinoor.ttc",
            "/System/Library/Fonts/KohinoorDevanagari.ttc",
            "/Library/Fonts/Kohinoor.ttc",
            "/System/Library/Fonts/Supplemental/Devanagari MT.ttf",
        ],
    }

    # Default Latin fonts
    latin_fonts = [
        "/System/Library/Fonts/Helvetica.ttc",
        "/System/Library/Fonts/SFNSText.ttf",
        "/System/Library/Fonts/SFNS.ttf",
        "/Library/Fonts/Arial.ttf",
        "/System/Library/Fonts/Supplemental/Arial.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        "C:\\Windows\\Fonts\\arial.ttf",
    ]

    # Try script-specific fonts first
    if language and language in SCRIPT_FONTS:
        for path in SCRIPT_FONTS[language]:
            try:
                font = ImageFont.truetype(path, size)
                print(f"  Using font: {path}")
                return font
            except (IOError, OSError):
                continue

    # Fall back to Latin fonts
    for path in latin_fonts:
        try:
            return ImageFont.truetype(path, size)
        except (IOError, OSError):
            continue

    try:
        return ImageFont.truetype("Arial", size)
    except:
        print("Warning: No TrueType font found, using default bitmap font")
        return ImageFont.load_default()


def load_pua_map(language):
    """Load PUA codepoint -> cluster text mapping for a language."""
    import csv as csv_mod
    map_path = Path(__file__).parent / "vocab" / f"{language}_pua_map.csv"
    if not map_path.exists():
        return {}
    pua_map = {}
    with open(map_path, encoding='utf-8') as f:
        reader = csv_mod.DictReader(f)
        for row in reader:
            # Parse "U+E000" -> 0xE000
            cp = int(row['pua_codepoint'].replace('U+', ''), 16)
            pua_map[cp] = row['cluster_text']
    return pua_map


def render_glyph(font, char, font_size, render_text=None):
    """Render a single character (or text string) and extract its metrics and alpha bitmap.

    If render_text is provided, it is rendered visually but stored under the codepoint of char.
    This is used for PUA glyphs where the display text is a multi-character cluster.
    """
    display = render_text if render_text else char
    canvas_size = font_size * 4  # Extra space for complex clusters
    img = Image.new('L', (canvas_size, canvas_size), 0)
    draw = ImageDraw.Draw(img)

    origin_x = canvas_size // 3
    origin_y = canvas_size // 3

    draw.text((origin_x, origin_y), display, fill=255, font=font)

    bbox = img.getbbox()
    if bbox is None:
        return None

    left, top, right, bottom = bbox
    width = right - left
    height = bottom - top

    char_bbox = font.getbbox(display)
    advance_width = char_bbox[2] - char_bbox[0]

    dx = left - origin_x
    ascent, descent = font.getmetrics()
    baseline_y = origin_y + ascent
    dy = baseline_y - top

    cropped = img.crop(bbox)
    pixels = list(cropped.getdata())

    return {
        'unicode': ord(char),
        'width': width,
        'height': height,
        'xAdvance': max(advance_width, width + max(0, dx)),
        'dY': dy,
        'dX': dx,
        'bitmap': pixels,
    }


def generate_vlw(chars, font_size, output_path, font_label="Font", language=None):
    """Generate a .vlw font file."""
    print(f"Generating {output_path.name} (size {font_size}, {len(chars)} chars)...")

    font = get_system_font(font_size, language=language)
    ascent, descent = font.getmetrics()

    # Load PUA map if available (for pre-shaped conjunct clusters)
    pua_map = load_pua_map(language) if language else {}
    if pua_map:
        print(f"  PUA map loaded: {len(pua_map)} conjunct clusters")

    glyphs = []
    for char in chars:
        cp = ord(char)
        render_text = pua_map.get(cp)  # None for regular chars, cluster text for PUA
        glyph = render_glyph(font, char, font_size, render_text=render_text)
        if glyph is not None:
            glyphs.append(glyph)
        else:
            glyphs.append({
                'unicode': cp,
                'width': 0,
                'height': 0,
                'xAdvance': font_size // 4,
                'dY': 0,
                'dX': 0,
                'bitmap': [],
            })

    glyphs.sort(key=lambda g: g['unicode'])

    gCount = len(glyphs)
    version = 0x0B

    print(f"  Glyphs: {gCount}, Ascent: {ascent}, Descent: {descent}")

    data = bytearray()

    # Header: 6 x uint32_t BE
    data += struct.pack('>I', gCount)
    data += struct.pack('>I', version)
    data += struct.pack('>I', font_size)
    data += struct.pack('>I', 0)
    data += struct.pack('>I', ascent)
    data += struct.pack('>I', descent)

    # Glyph metrics
    for g in glyphs:
        data += struct.pack('>i', g['unicode'])
        data += struct.pack('>i', g['height'])
        data += struct.pack('>i', g['width'])
        data += struct.pack('>i', g['xAdvance'])
        data += struct.pack('>i', g['dY'])
        data += struct.pack('>i', g['dX'])
        data += struct.pack('>i', 0)

    # Bitmap data
    for g in glyphs:
        for pixel in g['bitmap']:
            data += struct.pack('B', pixel)

    # Footer
    name_bytes = font_label.encode('ascii', errors='replace')
    data += struct.pack('B', len(name_bytes))
    data += name_bytes + b'\x00'
    data += struct.pack('B', len(name_bytes))
    data += name_bytes + b'\x00'
    data += struct.pack('B', 1)

    output_path.parent.mkdir(parents=True, exist_ok=True)
    with open(output_path, 'wb') as f:
        f.write(data)

    file_size = len(data)
    print(f"  Written: {output_path} ({file_size} bytes)")

    special = [g for g in glyphs if g['unicode'] > 0x7F]
    if special:
        print(f"  Special chars: {', '.join(chr(g['unicode']) for g in special)}")

    return file_size


def main():
    parser = argparse.ArgumentParser(description="Generate VLW font files for TFT_eSPI")
    parser.add_argument("--language", default="spanish",
                       choices=list(LANGUAGE_CHARS.keys()),
                       help="Language for special characters")
    parser.add_argument("--size", type=int, default=26,
                       help="Font size in points")
    parser.add_argument("--output", type=str, default=None,
                       help="Output file path (default: data/font.vlw)")
    args = parser.parse_args()

    if args.output:
        output_path = Path(args.output)
    else:
        project_root = Path(__file__).parent.parent
        output_path = project_root / "data" / "font.vlw"

    chars = get_char_set(args.language)
    size = generate_vlw(chars, args.size, output_path, f"{args.language}Font", language=args.language)
    print(f"\nTotal: {size} bytes ({size / 1024:.1f} KB)")


if __name__ == "__main__":
    main()
