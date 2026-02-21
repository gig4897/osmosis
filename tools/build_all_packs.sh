#!/bin/bash
# Build all language packs from vocab CSVs
# Usage: ./tools/build_all_packs.sh

set -e
cd "$(dirname "$0")/.."

echo "=== Building all language packs (5 languages x 4 tiers = 20 packs) ==="

LANGUAGES="spanish french portuguese chinese dutch"
TIERS="beginner intermediate advanced expert"

for lang in $LANGUAGES; do
    for tier in $TIERS; do
        csv="tools/vocab/${lang}_${tier}.csv"
        if [ -f "$csv" ]; then
            python3 tools/build_pack.py --csv "$csv" --output "packs/$lang/$tier/" --language "$lang" --tier "$tier"
        else
            echo "WARNING: $csv not found, skipping"
        fi
    done
done

# Generate fonts for each language
for lang in $LANGUAGES; do
    python3 tools/generate_vlw_font.py --language "$lang" --size 26 --output "packs/$lang/font.vlw"
done

echo "=== All packs built ==="
