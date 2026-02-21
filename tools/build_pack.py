#!/usr/bin/env python3
"""Build a language pack from a vocab CSV file.

Usage:
    python3 tools/build_pack.py --csv tools/vocab/spanish_beginner.csv \
        --output packs/spanish/beginner/ --language spanish --tier beginner
"""

import argparse
import csv
import json
from pathlib import Path


LANG_DISPLAY = {
    "spanish": "Spanish", "french": "French",
    "portuguese": "Portuguese", "chinese": "Chinese",
    "dutch": "Dutch"
}
TIER_DISPLAY = {
    "beginner": "Beginner", "intermediate": "Intermediate",
    "advanced": "Advanced", "expert": "Expert"
}


def build_manifest(csv_path, output_dir, language, tier):
    """Read vocab CSV and generate manifest.json."""
    words = []
    with open(csv_path) as f:
        reader = csv.DictReader(f)
        for row in reader:
            words.append({
                "word": row["translation"],
                "english": row["english"],
                "phonetic": row["phonetic"],
                "emoji": row["emoji"],
                "category": row["category"],
            })

    manifest = {
        "language": language,
        "languageDisplay": LANG_DISPLAY.get(language, language.title()),
        "tier": tier,
        "tierDisplay": TIER_DISPLAY.get(tier, tier.title()),
        "version": 1,
        "wordCount": len(words),
        "fontFile": "font.vlw",
        "words": words,
    }

    output_dir = Path(output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    manifest_path = output_dir / "manifest.json"
    with open(manifest_path, 'w') as f:
        json.dump(manifest, f, indent=2, ensure_ascii=False)

    print(f"Written {manifest_path} ({len(words)} words)")
    return manifest


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Build a language pack manifest")
    parser.add_argument("--csv", required=True, help="Path to vocab CSV file")
    parser.add_argument("--output", required=True, help="Output directory for pack files")
    parser.add_argument("--language", required=True, help="Language ID (e.g. spanish)")
    parser.add_argument("--tier", required=True, help="Tier ID (e.g. beginner)")
    args = parser.parse_args()
    build_manifest(args.csv, args.output, args.language, args.tier)
