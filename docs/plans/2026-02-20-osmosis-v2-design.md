# Osmosis v2.0 Design Document

**Date:** 2026-02-20
**Status:** Approved

## Overview

Osmosis v2.0 evolves from a standalone single-language Spanish flashcard device into a multi-language learning platform with cloud-backed content delivery, WiFi connectivity, and downloadable language packs.

**Launch languages:** Spanish, French, Portuguese, Chinese (Mandarin), Dutch
**Content structure:** 3 tiers per language (Beginner / Intermediate / Advanced, ~300 words each)

## Architecture

Three layers:

1. **Content Pipeline** (build-time Python toolchain) -- generates language packs
2. **VCodeWorks.dev** (static CDN) -- hosts packs and landing page
3. **ESP32 Firmware v2.0** -- WiFi-enabled device with pack downloader

## Key Design Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Connectivity | WiFi on device | ESP32 has WiFi hardware; enables seamless pack downloads |
| Backend | Static CDN (Cloudflare Pages) | Simple, cheap, infinitely scalable, no server maintenance |
| Content scope | Tiered packs (300 words x 3 tiers) | Manageable creation effort, clear learning progression |
| Pronunciation | Text-based phonetics | No audio hardware; toggle on/off in settings |
| WiFi setup | Captive portal | Standard IoT pattern; no typing on tiny screen |
| Storage | One pack at a time | Fits 4MB flash; re-download to switch languages |
| Chinese display | Pinyin only (no hanzi) | Hanzi font would be 500KB+; pinyin uses Latin chars |
| Pack switching | Resets progress | Simpler; 300-word packs are short enough to restart |
| Emoji naming | By codepoint (e.g. 1f4a7.bin) | Language-independent; shared across all packs |
| Image quality | 96x96 source, 120x120 display | Higher quality than v1 (was 72x72) |

---

## Layer 1: Content Pipeline

### Master Vocabulary Format

Per-language CSV file per tier (15 total for launch):

```
# tools/vocab/spanish_beginner.csv
emoji,english,translation,phonetic,category
1f4a7,water,agua,AH-gwah,nature
1f3e0,house,casa,KAH-sah,home
```

### Shared Emoji Image Set

Emoji are universal across languages. Rendered once from Apple Color Emoji:
- Source: 96x96 PNG via PyObjC (NSFont)
- Compressed: ORLE RGB565 .bin format (~6KB each)
- Named by Unicode codepoint: `1f4a7.bin`
- ~500-600 unique emoji across all 15 packs

### Per-Language Font Generation

VLW smooth font files with language-specific character sets:

| Language | Extra chars | Approx font size |
|----------|------------|-------------------|
| Spanish | a e i o u n u + accented | ~30KB |
| French | a a c e e e e i i o u u u y ae oe | ~35KB |
| Portuguese | a a a a c e e i o o o u | ~30KB |
| Dutch | e e i u (minimal) | ~25KB |
| Chinese | Pinyin tone marks (a/e/i/o/u x 4 tones) | ~30KB |

### Pack Output Structure

```
packs/
  catalog.json
  emoji/
    1f4a7.bin, 1f3e0.bin, ...
  {language}/
    {tier}/
      manifest.json
      font.vlw
```

### manifest.json Schema

```json
{
  "language": "spanish",
  "languageDisplay": "Spanish",
  "tier": "beginner",
  "tierDisplay": "Beginner",
  "version": 1,
  "wordCount": 300,
  "fontFile": "font.vlw",
  "words": [
    {
      "word": "agua",
      "english": "water",
      "phonetic": "AH-gwah",
      "emoji": "1f4a7",
      "category": "nature"
    }
  ]
}
```

---

## Layer 2: VCodeWorks.dev (Static CDN)

### Hosting

Static site on Cloudflare Pages. Domain: vcodeworks.dev

### URL Structure

```
vcodeworks.dev/
  index.html                           # Landing page
  support/                             # Support pages
  api/osmosis/
    catalog.json                       # Master pack catalog
    packs/
      emoji/{codepoint}.bin            # Shared emoji images
      {language}/{tier}/manifest.json  # Pack vocabulary
      {language}/{tier}/font.vlw       # Language font
```

### catalog.json Schema

```json
{
  "apiVersion": 1,
  "baseUrl": "https://vcodeworks.dev/api/osmosis/packs",
  "emojiVersion": 1,
  "emojiCount": 600,
  "languages": [
    {
      "id": "spanish",
      "name": "Spanish",
      "flag": "ES",
      "tiers": [
        {
          "id": "beginner",
          "name": "Beginner",
          "words": 300,
          "version": 1,
          "manifestSize": 28000,
          "fontFile": "font.vlw",
          "fontSize": 35000
        }
      ]
    }
  ]
}
```

### Download Flow

1. ESP32 fetches catalog.json (~2KB)
2. User selects language + tier on device
3. ESP32 fetches manifest.json (~30KB) + font.vlw (~30KB)
4. ESP32 fetches emoji .bin files, skipping any already on SPIFFS
5. ESP32 deletes orphaned emoji files not in new manifest
6. Total fresh install: ~1.86MB. Language switch with shared emoji: ~100KB.

---

## Layer 3: ESP32 Firmware v2.0

### Partition Scheme

Custom partition table with OTA support:

```
nvs,       data, nvs,     0x9000,   0x5000    # 20KB settings
otadata,   data, ota,     0xE000,   0x2000    # 8KB OTA metadata
app0,      app,  ota_0,   0x10000,  0xC0000   # 768KB firmware slot 1
app1,      app,  ota_1,   0xD0000,  0xC0000   # 768KB firmware slot 2
spiffs,    data, spiffs,  0x190000, 0x270000  # 2.5MB SPIFFS
```

### New Firmware Modules

#### wifi_manager.cpp/h
States: NotConfigured, CaptivePortalActive, Connecting, Connected, Disconnected.
Captive portal creates AP "Osmosis-Setup", serves HTML config page, saves credentials to NVS.

#### pack_manager.cpp/h
Manages download, install, and tracking of content packs. HTTP client fetches from CDN. Shows progress bar during download. Handles incremental emoji downloads.

#### vocab_loader.cpp/h
Parses /manifest.json from SPIFFS at boot. Builds WordEntry array in heap (~40KB for 300 words). Replaces compile-time word_data.h.

### Expanded WordEntry Struct

```cpp
struct WordEntry {
    char word[32];       // Foreign word
    char english[32];    // English translation
    char phonetic[40];   // Phonetic pronunciation
    char emoji[12];      // Codepoint hex string
    char category[16];   // Category
};
```

### Expanded Settings

```cpp
struct OsmosisSettings {
    // Existing
    uint8_t  wordsPerDay;
    uint16_t displaySecs;
    uint8_t  brightness;
    uint16_t progressIndex;
    uint8_t  lastDay;
    // v2.0
    bool     showPhonetic;
    bool     wifiConfigured;
    char     wifiSSID[33];
    char     wifiPass[65];
    char     installedLang[16];
    char     installedTier[16];
    uint8_t  installedVer;
};
```

### New UI Screens

1. **No Pack Installed** -- shown at boot if no manifest.json on SPIFFS
2. **WiFi Setup Prompt** -- instructions to connect to captive portal
3. **Language Browser** -- scrollable list from catalog.json with language + tier selection
4. **Download Progress** -- full-screen progress bar during pack install
5. **Updated Settings** -- adds Language, WiFi, and Phonetic toggle sections

### Updated Card Screen

- Header subtitle shows installed language name from manifest
- Word box: foreign word (smooth font) + English + optional phonetic
- Phonetic text appears below English when setting enabled
- Images loaded by codepoint (`emoji` field) instead of word-based filename

### Image Renderer Changes

- Load by codepoint: `imageRenderer::preloadImage("1f4a7")` instead of `preloadImage("agua")`
- Source images now 96x96, displayed at 120x120 via nearest-neighbor
- Update IMG_W/IMG_H constants to 96

---

## Language-Specific Notes

### Chinese (Mandarin)
- Displays pinyin romanization as the "word" (e.g. "shui" for water)
- Phonetic shows tone numbers (e.g. "shui3")
- Font includes tone-marked vowels (macron, acute, caron, grave accents)
- No hanzi characters rendered on device

### French
- Broader accent set than Spanish (ae, oe ligatures, cedilla, circumflex, etc.)
- Larger VLW font file (~35KB)

### Portuguese
- Very similar character set to Spanish
- Could potentially share font file with Spanish

### Dutch
- Mostly ASCII, minimal accent characters
- Smallest font file

---

## Implementation Order (Suggested)

1. Content pipeline (vocab CSVs + emoji rendering + font generation + manifest builder)
2. Static site scaffolding (VCodeWorks.dev landing + pack file hosting)
3. ESP32 WiFi manager + captive portal
4. ESP32 pack manager + download UI
5. ESP32 vocab loader (dynamic word data from manifest)
6. ESP32 updated card screen + settings UI
7. Polish card screen to match mockup precisely
8. Generate all 15 language packs (5 languages x 3 tiers)
9. Integration testing + OTA update support
