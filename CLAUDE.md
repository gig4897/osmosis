# Osmosis - ESP32 Language Flashcard Device

## Project Overview
Osmosis is an ESP32-based handheld language learning device with a 2.8" ILI9341 TFT touchscreen (240x320, portrait, RGB565). It displays flashcards with emoji images, foreign words, phonetic pronunciation, and English translations. Language packs are downloaded over WiFi from a static CDN.

## Repository Locations

| Repo | Path | Remote |
|------|------|--------|
| **Firmware** (this repo) | `/Users/kevintomlinson/Coding/Osmosis/` | `https://github.com/gig4897/osmosis.git` |
| **CDN/Website** | `/Users/kevintomlinson/Coding/vcwebsite/` | `https://github.com/gig4897/vcwebsite.git` |

The CDN is hosted on **Cloudflare Pages** and auto-deploys when you push to `gig4897/vcwebsite`. The live URL is `https://www.vcodeworks.dev/api/osmosis`.

## Build & Flash Firmware

```bash
cd /Users/kevintomlinson/Coding/Osmosis

# Build only
pio run

# Build and flash to device (auto-detects /dev/cu.usbserial-110)
pio run -t upload

# Serial monitor (pio device monitor has termios issues in some shells, use Python instead)
python3 -c "
import serial, sys
ser = serial.Serial('/dev/cu.usbserial-110', 115200, timeout=1)
ser.dtr = False; ser.rts = False  # Don't reset ESP32
while True:
    line = ser.readline()
    if line: sys.stdout.write(line.decode('utf-8', errors='replace')); sys.stdout.flush()
"
```

### Key Build Facts
- PlatformIO with ESP-IDF Arduino framework
- SPIFFS filesystem (~1400KB available for packs + emoji)
- Custom partition table: `partitions_ota.csv`
- TFT_eSPI configured via build flags in `platformio.ini` (not User_Setup.h)
- Touch uses separate HSPI bus (XPT2046_Touchscreen library, not TFT_eSPI touch)

## Creating & Deploying Language Packs

### Step 1: Create Vocabulary CSV
Add a CSV file to `tools/vocab/` with this format:
```csv
emoji,english,translation,phonetic,category
1f4a7,water,agua,AH-gwah,nature
1f3e0,house,casa,KAH-sah,home
```
- `emoji`: Unicode codepoint hex (e.g., `1f4a7` for water droplet)
- `phonetic`: All-caps pronunciation guide
- `category`: Grouping tag (nature, food, animals, etc.)

### Step 2: Build the Pack
```bash
# Build a single pack
python3 tools/build_pack.py \
  --csv tools/vocab/spanish_beginner.csv \
  --output packs/spanish/beginner/ \
  --language spanish --tier beginner

# Or build ALL packs at once
./tools/build_all_packs.sh
```
This generates `manifest.json` in each pack's output directory.

### Step 3: Generate the Font
```bash
python3 tools/generate_vlw_font.py \
  --language spanish --size 26 \
  --output packs/spanish/font.vlw
```
One font per language (shared across tiers). The font includes ASCII + language-specific accented characters.

### Step 4: Generate Emoji .bin Files (if new emoji needed)
```bash
# Render emoji using Apple Color Emoji (macOS only, requires pyobjc)
pip install pyobjc pyobjc-framework-Cocoa
python3 tools/render_apple_emoji.py tools/emoji_apple_png/ --csv tools/vocab/

# Convert PNG to ORLE-compressed RGB565 .bin
python3 tools/convert_emoji.py tools/emoji_apple_png/ tools/emoji_bin/
```
- Input: 96x96 PNG on black background
- Output: ORLE-compressed RGB565 `.bin` files (~3-6KB each)
- Black pixels (0x0000) = transparent in firmware

### Step 5: Deploy to CDN
```bash
cd /Users/kevintomlinson/Coding/vcwebsite

# Copy pack manifest + font
cp /path/to/packs/spanish/beginner/manifest.json api/osmosis/packs/spanish/beginner/
cp /path/to/packs/spanish/font.vlw api/osmosis/packs/spanish/beginner/

# Copy any new emoji .bin files
cp tools/emoji_bin/*.bin api/osmosis/packs/emoji/

# Update catalog.json if adding a new language or tier
# (manually edit api/osmosis/catalog.json)

# Push to deploy
git add . && git commit -m "Add new pack" && git push
```
Cloudflare Pages deploys automatically within ~1 minute.

### Step 6: Update catalog.json (for new languages/tiers)
Edit `/Users/kevintomlinson/Coding/vcwebsite/api/osmosis/catalog.json` to add the new entry. The device fetches this first to populate the language browser.

## CDN URL Structure
```
https://www.vcodeworks.dev/api/osmosis/
  catalog.json                              # Language/tier listing
  packs/{lang}/{tier}/manifest.json         # Word list + metadata
  packs/{lang}/{tier}/font.vlw              # TFT_eSPI smooth font
  packs/emoji/{codepoint}.bin               # Shared emoji images
```

## Current Language Packs
| Language | Flag | Tiers | Words per Tier |
|----------|------|-------|---------------|
| Spanish | ES | Beginner, Intermediate, Advanced, Expert, Numbers | 75 each (375 total) |
| French | FR | Beginner, Intermediate, Advanced, Expert, Numbers | 75 each (375 total) |
| Portuguese (BR) | BR | Beginner, Intermediate, Advanced, Expert, Numbers | 75 each (375 total) |
| Portuguese (PT) | PT | Beginner, Intermediate, Advanced, Expert, Numbers | 75 each (375 total) |
| Chinese | CN | Beginner, Intermediate, Advanced, Expert, Numbers | 75 each (375 total) |
| Dutch | NL | Beginner, Intermediate, Advanced, Expert, Numbers | 75 each (375 total) |
| Hindi | IN | Beginner, Intermediate, Advanced, Expert, Numbers | 75 each (375 total) |
| Arabic | EG | Beginner, Intermediate, Advanced, Expert, Numbers | 75 each (375 total) |
| Urdu | PK | Beginner, Intermediate, Advanced, Expert, Numbers | 75 each (375 total) |
| Japanese | JP | Beginner, Intermediate, Advanced, Expert, Numbers | 75 each (375 total) |
| Korean | KR | Beginner, Intermediate, Advanced, Expert, Numbers | 75 each (375 total) |
| Q'eqchi' | GT | Beginner, Intermediate, Advanced, Expert, Numbers | 75 each (375 total) |
| Tsalagi (Cherokee) | US | Beginner, Intermediate, Advanced, Expert, Numbers | 75 each (375 total) |
| Mvskoke (Creek) | US | Beginner, Intermediate, Advanced, Expert, Numbers | 75 each (375 total) |

**70 packs total** (14 languages × 5 tiers), **5,250 words**, **343 emoji .bin files** on CDN as of Feb 2026.

**Product website:** https://www.vcodeworks.dev/osmosis/
**Word lists:** https://www.vcodeworks.dev/osmosis/words.html

## Architecture Notes

### Display Rendering
- **Strip-based rendering**: 240x10 pixel TFT_eSprite, 32 strips per frame
- Text that crosses strip boundaries must check `y >= -FONT_HEIGHT && y < STRIP_H`
- TFT_eSPI built-in fonts: Font 1 (8px), Font 2 (16px), Font 4 (26px)
- Smooth fonts (VLW) loaded from SPIFFS for accented characters

### Device State Machine
```
NoPack → (download) → Downloading → (complete) → ESP.restart() → Cards
Cards ↔ Settings (long press toggles)
NoPack ↔ Settings (long press toggles)
```
After a successful pack download, the device reboots (`ESP.restart()`) to ensure full heap (~229KB) is available for JSON parsing.

### Key Technical Details
- HTTP downloads use `http.writeToStream()` (NOT `getStreamPtr()`) to properly handle Cloudflare's chunked transfer encoding
- On boot, if `vocabLoader::load()` fails, the corrupt `/manifest.json` is deleted
- WiFi connects via captive portal (user enters SSID/password in settings)
- Settings stored in NVS (ESP32 Preferences library)
- Emoji are ORLE-compressed RGB565, rendered at 1.25x scale (96px stored, 120px displayed)

### Color Palette (RGB565)
| Name | Value | Use |
|------|-------|-----|
| CLR_BG_DARK | 0x0000 | Main background (black) |
| CLR_HEADER_BG | 0x1269 | Header bar |
| CLR_ACCENT | 0x4E1F | Accent/buttons (purple-blue) |
| CLR_TEXT_PRIMARY | 0xFFFF | Foreign word (white) |
| CLR_PHONETIC | 0xAD75 | Phonetic text (~gray 170) |
| CLR_ENGLISH | 0xDEDB | English word (~white 222) |
| CLR_TEXT_DIM | 0x6B4D | Counter, subtle text |
| CLR_WORD_BOX_BG | 0x0882 | Word box fill (dark blue) |

## File Guide
| File | Purpose |
|------|---------|
| `src/main.cpp` | Setup, state machine, main loop |
| `src/card_screen.cpp` | Flashcard rendering (emoji + word box) |
| `src/ui_settings.cpp` | Settings menu + language browser UI |
| `src/pack_manager.cpp` | Download manager (catalog, manifest, font, emoji) |
| `src/vocab_loader.cpp` | Parse manifest.json into WordEntry array |
| `src/wifi_manager.cpp` | WiFi connection + captive portal |
| `src/image_renderer.cpp` | ORLE emoji decompression + scaled rendering |
| `src/display_manager.cpp` | TFT_eSprite strip management + backlight |
| `src/constants.h` | Layout, colors, pins, timing constants |
| `tools/build_pack.py` | CSV → manifest.json |
| `tools/build_all_packs.sh` | Build all 70 packs + 14 fonts |
| `tools/render_apple_emoji.py` | Apple Color Emoji → PNG (macOS, pyobjc) |
| `tools/convert_emoji.py` | PNG → ORLE RGB565 .bin |
| `tools/generate_vlw_font.py` | Create TFT_eSPI VLW font files (supports auto char extraction for non-Latin scripts) |
| `tools/generate_wordlist_html.py` | Generate browsable word list HTML page from all vocab CSVs |
| `tools/preshape_hindi.py` | Pre-shape Hindi conjunct clusters to PUA codepoints for TFT rendering |
| `tools/restore_native_script.py` | One-time tool: restore native scripts + reshape Arabic/Urdu for LTR display |

## Known Issues / History
- Cloudflare Pages serves with `Transfer-Encoding: chunked` — must use `writeToStream()` not `getStreamPtr()` or files get corrupted with chunk headers
- ESP32 has ~229KB free heap on fresh boot but only ~22KB after a synchronous download — always reboot after pack install
- `pio device monitor` may not work in all terminal environments — use Python pyserial as shown above
