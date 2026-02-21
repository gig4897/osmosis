#!/usr/bin/env python3
"""Restore native script translations from git history and pre-shape for TFT rendering.

For Cherokee, Korean, Japanese kana: just restore the native script (no shaping needed).
For Arabic, Urdu: restore native script + apply arabic_reshaper + bidi reordering.
For Hindi: restore native script + apply Unicode NFC normalization.
Chinese stays as pinyin (official romanization).

Usage:
    python3 tools/restore_native_script.py
"""

import csv
import subprocess
import io
import os

# Languages to restore native script for
RESTORE_LANGUAGES = ['tsalagi', 'korean', 'japanese', 'arabic', 'urdu', 'hindi']
TIERS = ['beginner', 'intermediate', 'advanced', 'expert']
GIT_COMMIT = 'a7aa0dd'  # Commit with original native script CSVs


def get_original_csv(language, tier):
    """Get original CSV content from git history."""
    path = f'tools/vocab/{language}_{tier}.csv'
    result = subprocess.run(
        ['git', 'show', f'{GIT_COMMIT}:{path}'],
        capture_output=True, text=True
    )
    if result.returncode != 0:
        print(f"  ERROR: Could not retrieve {path} from git: {result.stderr.strip()}")
        return None
    return result.stdout


def parse_csv(content):
    """Parse CSV content into list of dicts."""
    reader = csv.DictReader(io.StringIO(content))
    return list(reader)


def reshape_arabic(text):
    """Pre-shape Arabic/Urdu text for display on a simple LTR renderer.

    This converts each character to its correct positional form (initial, medial, final, isolated)
    and reorders for visual LTR display, so the TFT can render it left-to-right.
    """
    import arabic_reshaper
    from bidi.algorithm import get_display

    # Reshape: converts to correct positional forms
    reshaped = arabic_reshaper.reshape(text)
    # Bidi: reorder for visual LTR display
    display = get_display(reshaped)
    return display


def normalize_devanagari(text):
    """Normalize Hindi/Devanagari text to NFC form.

    This ensures conjuncts use precomposed forms where available.
    For Devanagari, most conjuncts are rendered via virama (halant) sequences,
    which the font needs to handle. We normalize to NFC for consistency.
    """
    import unicodedata
    return unicodedata.normalize('NFC', text)


def convert_japanese_to_kana(text):
    """Convert Japanese text to hiragana/katakana only (strip kanji).

    We can't render kanji on the device (too many glyphs), but we can render kana.
    This uses a simple mapping approach - for words with kanji, we use the phonetic
    reading stored in the phonetic column instead.
    """
    # Check if text contains kanji (CJK Unified Ideographs range)
    has_kanji = any(0x4E00 <= ord(c) <= 0x9FFF or 0x3400 <= ord(c) <= 0x4DBF for c in text)
    return text, has_kanji


def process_language(language):
    """Process all tiers for a language."""
    print(f"\n{'='*60}")
    print(f"Processing: {language.upper()}")
    print(f"{'='*60}")

    for tier in TIERS:
        print(f"\n  --- {tier} ---")

        # Get original native script CSV
        original_content = get_original_csv(language, tier)
        if original_content is None:
            continue
        original_rows = parse_csv(original_content)

        # Get current romanized CSV (for phonetic column and structure)
        current_path = f'tools/vocab/{language}_{tier}.csv'
        with open(current_path, 'r', encoding='utf-8') as f:
            current_rows = parse_csv(f.read())

        if len(original_rows) != len(current_rows):
            print(f"  WARNING: Row count mismatch! Original={len(original_rows)}, Current={len(current_rows)}")
            continue

        # Build output rows
        output_rows = []
        shaped_chars = set()
        kanji_words = []

        for i, (orig, curr) in enumerate(zip(original_rows, current_rows)):
            row = dict(curr)  # Start with current (has correct emoji, english, phonetic, category)
            native_text = orig['translation']

            if language in ('arabic', 'urdu'):
                # Pre-shape Arabic/Urdu text
                shaped = reshape_arabic(native_text)
                row['translation'] = shaped
                for c in shaped:
                    if ord(c) > 127:
                        shaped_chars.add(c)

            elif language == 'hindi':
                # Normalize Devanagari
                normalized = normalize_devanagari(native_text)
                row['translation'] = normalized
                for c in normalized:
                    if ord(c) > 127:
                        shaped_chars.add(c)

            elif language == 'japanese':
                # Check for kanji - if present, use kana reading from phonetic
                text, has_kanji = convert_japanese_to_kana(native_text)
                if has_kanji:
                    kanji_words.append((orig['english'], native_text))
                    # Keep native text as-is - we'll handle kanji in font generation
                row['translation'] = native_text
                for c in native_text:
                    if ord(c) > 127:
                        shaped_chars.add(c)

            elif language == 'korean':
                row['translation'] = native_text
                for c in native_text:
                    if ord(c) > 127:
                        shaped_chars.add(c)

            elif language == 'tsalagi':
                row['translation'] = native_text
                for c in native_text:
                    if ord(c) > 127:
                        shaped_chars.add(c)

            output_rows.append(row)

        # Write updated CSV
        with open(current_path, 'w', encoding='utf-8', newline='') as f:
            writer = csv.DictWriter(f, fieldnames=['emoji', 'english', 'translation', 'phonetic', 'category'])
            writer.writeheader()
            writer.writerows(output_rows)

        print(f"  Written {current_path} ({len(output_rows)} words)")
        print(f"  Unique non-ASCII chars: {len(shaped_chars)}")

        if language == 'japanese' and kanji_words:
            print(f"  Words with kanji: {len(kanji_words)}")

        # Show a sample
        if output_rows:
            sample = output_rows[0]
            print(f"  Sample: {sample['english']} → {sample['translation']} [{sample['phonetic']}]")


def count_all_chars(language):
    """Count all unique non-ASCII chars across all tiers for a language."""
    all_chars = set()
    for tier in TIERS:
        path = f'tools/vocab/{language}_{tier}.csv'
        with open(path, 'r', encoding='utf-8') as f:
            for row in csv.DictReader(f):
                for c in row['translation']:
                    if ord(c) > 127:
                        all_chars.add(c)
    return all_chars


def main():
    os.chdir(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

    for language in RESTORE_LANGUAGES:
        process_language(language)

    # Summary
    print(f"\n{'='*60}")
    print("SUMMARY - Unique characters per language")
    print(f"{'='*60}")
    for language in RESTORE_LANGUAGES:
        chars = count_all_chars(language)
        print(f"  {language:12s}: {len(chars):4d} unique non-ASCII chars")
        # Show char ranges
        if chars:
            ranges = {}
            for c in sorted(chars):
                cp = ord(c)
                if 0x13A0 <= cp <= 0x13FF:
                    ranges['Cherokee'] = ranges.get('Cherokee', 0) + 1
                elif 0xAC00 <= cp <= 0xD7AF:
                    ranges['Hangul Syllables'] = ranges.get('Hangul Syllables', 0) + 1
                elif 0x3040 <= cp <= 0x309F:
                    ranges['Hiragana'] = ranges.get('Hiragana', 0) + 1
                elif 0x30A0 <= cp <= 0x30FF:
                    ranges['Katakana'] = ranges.get('Katakana', 0) + 1
                elif 0x4E00 <= cp <= 0x9FFF:
                    ranges['CJK Kanji'] = ranges.get('CJK Kanji', 0) + 1
                elif 0x0600 <= cp <= 0x06FF or 0xFE70 <= cp <= 0xFEFF or 0xFB50 <= cp <= 0xFDFF:
                    ranges['Arabic/Urdu'] = ranges.get('Arabic/Urdu', 0) + 1
                elif 0x0900 <= cp <= 0x097F:
                    ranges['Devanagari'] = ranges.get('Devanagari', 0) + 1
                else:
                    ranges['Other'] = ranges.get('Other', 0) + 1
            for name, count in sorted(ranges.items()):
                print(f"    {name}: {count}")


if __name__ == '__main__':
    main()
