#!/bin/bash
# Build all language packs from vocab CSVs
# Usage: ./tools/build_all_packs.sh

set -e
cd "$(dirname "$0")/.."

echo "=== Building all language packs ==="

# Spanish (3 tiers)
python3 tools/build_pack.py --csv tools/vocab/spanish_beginner.csv --output packs/spanish/beginner/ --language spanish --tier beginner
python3 tools/build_pack.py --csv tools/vocab/spanish_intermediate.csv --output packs/spanish/intermediate/ --language spanish --tier intermediate
python3 tools/build_pack.py --csv tools/vocab/spanish_advanced.csv --output packs/spanish/advanced/ --language spanish --tier advanced

# French
python3 tools/build_pack.py --csv tools/vocab/french_beginner.csv --output packs/french/beginner/ --language french --tier beginner

# Portuguese
python3 tools/build_pack.py --csv tools/vocab/portuguese_beginner.csv --output packs/portuguese/beginner/ --language portuguese --tier beginner

# Chinese
python3 tools/build_pack.py --csv tools/vocab/chinese_beginner.csv --output packs/chinese/beginner/ --language chinese --tier beginner

# Dutch
python3 tools/build_pack.py --csv tools/vocab/dutch_beginner.csv --output packs/dutch/beginner/ --language dutch --tier beginner

# Generate fonts for each language
for lang in spanish french portuguese chinese dutch; do
    python3 tools/generate_vlw_font.py --language "$lang" --size 26 --output "packs/$lang/font.vlw"
done

echo "=== All packs built ==="
