#!/usr/bin/env python3
"""Pre-shape Hindi (Devanagari) text by rendering conjunct clusters as PUA glyphs.

Devanagari conjuncts (like क्ष, त्र, etc.) are formed by combining base consonants
with a halant (virama U+094D). TFT_eSPI can't do OpenType shaping, so we:

1. Split each Hindi word into "syllable clusters" (segments between halants)
2. Any cluster containing a halant gets mapped to a Private Use Area (PUA) codepoint
3. The VLW font is generated with these PUA glyphs rendered as pre-shaped bitmaps
4. The CSV translation field is rewritten using PUA codepoints

This way the device just renders PUA glyphs left-to-right and they look correct.

Usage:
    python3 tools/preshape_hindi.py
"""

import csv
import re
import unicodedata
from pathlib import Path


# PUA range starts at U+E000
PUA_START = 0xE000

# Devanagari virama/halant
HALANT = '\u094D'

# Devanagari vowel signs (matras) - these combine with preceding consonant
MATRAS = set('\u093E\u093F\u0940\u0941\u0942\u0943\u0944\u0945\u0946\u0947\u0948\u0949\u094A\u094B\u094C')

# Devanagari anusvara and chandrabindu
MODIFIERS = set('\u0901\u0902\u0903\u093C')


def split_into_clusters(text):
    """Split Devanagari text into orthographic clusters.

    A cluster is a unit that should be rendered together:
    - A consonant + halant + consonant (+ more halant+consonant) + optional matra/modifier
    - A standalone vowel
    - A consonant + optional matra/modifier
    - Non-Devanagari characters (spaces, etc.)
    """
    clusters = []
    i = 0
    while i < len(text):
        c = text[i]
        cp = ord(c)

        # Non-Devanagari character (space, punctuation, etc.)
        if not (0x0900 <= cp <= 0x097F):
            clusters.append(c)
            i += 1
            continue

        # Start building a cluster
        cluster = c
        i += 1

        # If this is a consonant, absorb halant+consonant sequences and trailing matra/modifier
        if 0x0915 <= cp <= 0x0939 or cp == 0x0958 or cp == 0x0959 or cp == 0x095A or cp == 0x095B or cp == 0x095C or cp == 0x095D or cp == 0x095E or cp == 0x095F:
            # Absorb nukta if present
            if i < len(text) and text[i] in MODIFIERS:
                cluster += text[i]
                i += 1

            # Absorb halant + consonant sequences
            while i + 1 < len(text) and text[i] == HALANT:
                next_cp = ord(text[i + 1])
                if 0x0915 <= next_cp <= 0x0939 or next_cp in (0x0958, 0x0959, 0x095A, 0x095B, 0x095C, 0x095D, 0x095E, 0x095F):
                    cluster += text[i] + text[i + 1]
                    i += 2
                    # Absorb nukta after consonant
                    if i < len(text) and text[i] in MODIFIERS:
                        cluster += text[i]
                        i += 1
                else:
                    break

            # Absorb trailing halant (for half-forms)
            if i < len(text) and text[i] == HALANT:
                cluster += text[i]
                i += 1

            # Absorb matra (vowel sign)
            if i < len(text) and text[i] in MATRAS:
                cluster += text[i]
                i += 1

            # Absorb anusvara/chandrabindu
            if i < len(text) and text[i] in MODIFIERS:
                cluster += text[i]
                i += 1

        elif c in MATRAS or c in MODIFIERS:
            # Standalone matra or modifier (shouldn't normally happen)
            pass
        else:
            # Vowel - absorb any modifier
            if i < len(text) and text[i] in MODIFIERS:
                cluster += text[i]
                i += 1

        clusters.append(cluster)

    return clusters


def needs_shaping(cluster):
    """Check if a cluster contains halant sequences that need OpenType shaping."""
    return HALANT in cluster and len(cluster) > 1


def process_hindi():
    """Process all Hindi CSVs: split into clusters, map conjuncts to PUA."""
    vocab_dir = Path(__file__).parent / 'vocab'

    # First pass: collect all unique clusters that need shaping
    all_clusters = {}  # cluster_text -> PUA codepoint

    # Also collect simple (non-conjunct) Devanagari characters
    simple_chars = set()

    pua_next = PUA_START

    tiers = ['beginner', 'intermediate', 'advanced', 'expert', 'numbers']
    for tier in tiers:
        csv_path = vocab_dir / f'hindi_{tier}.csv'
        if not csv_path.exists():
            continue
        with open(csv_path, encoding='utf-8') as f:
            for row in csv.DictReader(f):
                text = unicodedata.normalize('NFC', row['translation'])
                clusters = split_into_clusters(text)
                for cluster in clusters:
                    if needs_shaping(cluster):
                        if cluster not in all_clusters:
                            all_clusters[cluster] = pua_next
                            pua_next += 1
                    else:
                        for c in cluster:
                            if ord(c) > 0x7E:
                                simple_chars.add(c)

    print(f"Unique conjunct clusters: {len(all_clusters)}")
    print(f"Simple Devanagari chars: {len(simple_chars)}")
    print(f"PUA range: U+{PUA_START:04X} - U+{pua_next - 1:04X}")

    # Show some examples
    for cluster, pua in sorted(all_clusters.items(), key=lambda x: x[1])[:20]:
        codes = ' '.join(f'U+{ord(c):04X}' for c in cluster)
        print(f"  {cluster:10s} -> U+{pua:04X}  ({codes})")

    # Second pass: rewrite CSVs with PUA codepoints
    for tier in tiers:
        csv_path = vocab_dir / f'hindi_{tier}.csv'
        rows = []
        with open(csv_path, encoding='utf-8') as f:
            rows = list(csv.DictReader(f))

        for row in rows:
            text = unicodedata.normalize('NFC', row['translation'])
            clusters = split_into_clusters(text)
            new_text = ''
            for cluster in clusters:
                if needs_shaping(cluster) and cluster in all_clusters:
                    new_text += chr(all_clusters[cluster])
                else:
                    new_text += cluster
            row['translation'] = new_text

        with open(csv_path, 'w', encoding='utf-8', newline='') as f:
            writer = csv.DictWriter(f, fieldnames=['emoji', 'english', 'translation', 'phonetic', 'category'])
            writer.writeheader()
            writer.writerows(rows)

        print(f"Written: hindi_{tier}.csv")

    # Save cluster mapping for font generation
    mapping_path = vocab_dir / 'hindi_pua_map.csv'
    with open(mapping_path, 'w', encoding='utf-8', newline='') as f:
        writer = csv.writer(f)
        writer.writerow(['pua_codepoint', 'cluster_text', 'cluster_codes'])
        for cluster, pua in sorted(all_clusters.items(), key=lambda x: x[1]):
            codes = ' '.join(f'U+{ord(c):04X}' for c in cluster)
            writer.writerow([f'U+{pua:04X}', cluster, codes])

    print(f"\nPUA mapping saved to: {mapping_path}")
    return all_clusters, simple_chars


if __name__ == '__main__':
    process_hindi()
