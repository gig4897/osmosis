#!/usr/bin/env python3
"""Download Twemoji images and convert to ESP32 SPIFFS format."""

import os
import re
import sys
import subprocess
import urllib.request
from pathlib import Path

TWEMOJI_BASE = "https://raw.githubusercontent.com/twitter/twemoji/master/assets/72x72"
PROJECT_ROOT = Path(__file__).parent.parent
WORD_DATA_H = PROJECT_ROOT / "src" / "word_data.h"
PNG_DIR = PROJECT_ROOT / "tools" / "emoji_png"
DATA_DIR = PROJECT_ROOT / "data"
CONVERTER = PROJECT_ROOT / "tools" / "convert_emoji.py"

# Fallback emoji for codepoints missing from Twemoji (newer Unicode additions).
# Maps imageFile name -> fallback codepoint string for Twemoji URL.
EMOJI_FALLBACKS = {
    "empujar": "1f449",  # U+1FAF8 pushing hand -> U+1F449 pointing right
}


def decode_c_string(s):
    """Decode C-style escape sequences (\\xHH) to raw bytes, then UTF-8.

    Returns (decoded_string, success). On failure, returns (raw_string, False).
    """
    # The file contains C string literals with \\xHH escape sequences.
    # When read as text, these are literal characters: backslash, x, H, H.
    # We need to convert them to actual bytes and then decode as UTF-8.
    result = bytearray()
    i = 0
    while i < len(s):
        if s[i] == '\\' and i + 1 < len(s) and s[i + 1] == 'x':
            # Parse \\xHH
            hex_str = s[i + 2:i + 4]
            result.append(int(hex_str, 16))
            i += 4
        else:
            result.append(ord(s[i]))
            i += 1
    try:
        return result.decode('utf-8'), True
    except UnicodeDecodeError:
        return s, False


def codepoint_from_comment(line):
    """Extract Unicode codepoint(s) from the // comment in a word_data.h line.

    Returns the emoji string, or None if no codepoint found.
    """
    match = re.search(r'U\+([0-9A-Fa-f]+)', line)
    if match:
        try:
            return chr(int(match.group(1), 16))
        except (ValueError, OverflowError):
            pass
    return None


def parse_word_data():
    """Parse word_data.h and extract (imageFile, emoji_unicode) pairs."""
    content = WORD_DATA_H.read_text(encoding='utf-8')
    entries = []
    seen_image_files = set()

    for line in content.split('\n'):
        line = line.strip()
        if not line.startswith('{"'):
            continue

        # Extract all quoted strings from the line
        strings = re.findall(r'"((?:[^"\\]|\\.)*)"', line)

        if len(strings) == 5:
            # Normal: {"spanish", "english", "imageFile", "emoji", "category"}
            image_file = strings[2]
            emoji_raw = strings[3]
        elif len(strings) == 6:
            # C string concatenation in spanish field:
            # {"spa\xC3\xB1" "ish", "english", "imageFile", "emoji", "category"}
            image_file = strings[3]
            emoji_raw = strings[4]
        else:
            print(f"  WARNING: Skipping line with {len(strings)} strings: {line[:80]}")
            continue

        # Decode the C escape sequences in the emoji field
        emoji_unicode, ok = decode_c_string(emoji_raw)
        if not ok:
            # Fall back to codepoint from comment
            fallback = codepoint_from_comment(line)
            if fallback:
                emoji_unicode = fallback
                print(f"  NOTE: Used comment codepoint for {emoji_raw[:20]}")
            else:
                print(f"  WARNING: Could not decode emoji: {emoji_raw[:40]}")
                continue

        # Decode imageFile too (it may have \\x sequences, though typically ASCII)
        image_file_decoded, _ = decode_c_string(image_file)

        # Skip duplicates (same imageFile used for different words)
        if image_file_decoded in seen_image_files:
            continue
        seen_image_files.add(image_file_decoded)

        entries.append((image_file_decoded, emoji_unicode))

    return entries


def emoji_to_codepoints(emoji_str):
    """Convert an emoji string to dash-separated hex codepoints.

    Strips variation selector U+FE0F since Twemoji URLs usually omit it.
    Returns the codepoint string and a list of URL variants to try.
    """
    codepoints = []
    codepoints_with_vs = []
    for char in emoji_str:
        cp = ord(char)
        if cp < 0x20:
            continue
        codepoints_with_vs.append(f"{cp:x}")
        if cp == 0xFE0F:
            continue
        codepoints.append(f"{cp:x}")

    return "-".join(codepoints), "-".join(codepoints_with_vs)


def download_emoji(name, emoji_str, png_dir):
    """Download a Twemoji PNG for the given emoji."""
    out_path = png_dir / f"{name}.png"
    if out_path.exists():
        return True, "cached"

    without_vs, with_vs = emoji_to_codepoints(emoji_str)

    urls_to_try = []

    # Primary: without variation selector
    urls_to_try.append(f"{TWEMOJI_BASE}/{without_vs}.png")

    # With variation selector
    if with_vs != without_vs:
        urls_to_try.append(f"{TWEMOJI_BASE}/{with_vs}.png")

    # For compound emoji, try just the first codepoint
    if "-" in without_vs:
        first_cp = without_vs.split("-")[0]
        urls_to_try.append(f"{TWEMOJI_BASE}/{first_cp}.png")

    # Check for known fallback substitutions
    if name in EMOJI_FALLBACKS:
        urls_to_try.append(f"{TWEMOJI_BASE}/{EMOJI_FALLBACKS[name]}.png")

    for url in urls_to_try:
        try:
            urllib.request.urlretrieve(url, out_path)
            return True, url.split("/")[-1]
        except Exception:
            continue

    return False, f"tried: {', '.join(u.split('/')[-1] for u in urls_to_try)}"


def main():
    print("Osmosis Emoji Downloader")
    print("=" * 50)

    PNG_DIR.mkdir(parents=True, exist_ok=True)
    DATA_DIR.mkdir(parents=True, exist_ok=True)

    entries = parse_word_data()
    print(f"Found {len(entries)} unique image entries in word_data.h\n")

    success = 0
    failed = []

    for i, (name, emoji) in enumerate(entries):
        ok, detail = download_emoji(name, emoji, PNG_DIR)
        if ok:
            success += 1
            print(f"  [{i+1:3d}/{len(entries)}] {name:20s} OK  ({detail})")
        else:
            failed.append((name, emoji, detail))
            cps = " ".join(f"U+{ord(c):04X}" for c in emoji)
            print(f"  [{i+1:3d}/{len(entries)}] {name:20s} FAILED  {cps}  ({detail})")

    print(f"\nDownloaded: {success}/{len(entries)}")
    if failed:
        print(f"\nFailed ({len(failed)}):")
        for name, emoji, detail in failed:
            cps = " ".join(f"U+{ord(c):04X}" for c in emoji)
            print(f"  {name}: {cps} ({detail})")

    # Convert PNGs to .bin files
    print(f"\n{'=' * 50}")
    print("Converting PNGs to SPIFFS format...")
    print("=" * 50)
    result = subprocess.run(
        [sys.executable, str(CONVERTER), str(PNG_DIR), str(DATA_DIR)],
        capture_output=True, text=True
    )
    print(result.stdout)
    if result.stderr:
        # Filter out PIL warnings (non-fatal)
        for line in result.stderr.strip().split('\n'):
            if 'UserWarning' not in line and 'warnings.warn' not in line:
                print(f"  stderr: {line}")

    # Report total SPIFFS usage
    total_size = sum(f.stat().st_size for f in DATA_DIR.glob("*.bin"))
    print(f"\nTotal SPIFFS usage: {total_size / 1024:.1f}KB / 1500KB")
    if total_size > 1500 * 1024:
        print("WARNING: Total exceeds SPIFFS capacity!")
    else:
        print(f"Remaining: {(1500 * 1024 - total_size) / 1024:.1f}KB")

    print("\nDone!")


if __name__ == "__main__":
    main()
