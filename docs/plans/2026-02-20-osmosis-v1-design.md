# Osmosis v1.0 - Design Document

## Overview

Osmosis is an on-desk foreign language learning device. It passively displays flashcards with emoji images and word translations, rotating automatically so the user absorbs vocabulary through ambient exposure. v1.0 ships with Spanish.

**By VCodeworks LLC**

## Hardware

Same board as Tide Clock 3.0 (ESP32-dev):

- ESP32 (esp32dev), Arduino framework, PlatformIO
- ILI9341 2.8" TFT LCD, 240x320, 16-bit RGB565
- XPT2046 touchscreen (long press for settings only)
- PWM backlight on GPIO 21
- SPIFFS filesystem (~1.5MB with huge_app partition)

Pin configuration identical to Tide Clock 3.0:

| Pin     | GPIO |
|---------|------|
| TFT_MOSI | 13 |
| TFT_SCLK | 14 |
| TFT_CS   | 15 |
| TFT_DC   | 2  |
| TFT_RST  | 4  |
| TFT_BL   | 21 |
| TOUCH_CS | 33 |
| TOUCH_CLK | 25 |
| TOUCH_MOSI | 32 |
| TOUCH_MISO | 39 |

## Screens

### Splash Screen

Shown on boot for ~3 seconds:

- "OSMOSIS" title with glow effect
- "LEARN BY IMMERSION" tagline
- Animated loading dots
- "v1.0 - Spanish"
- "by VCodeworks LLC"

### Flashcard Screen

- 120x120 emoji bitmap centered in upper portion
- Spanish word in large white text below
- English translation smaller underneath in muted color
- Card counter at bottom (e.g. "Card 42 / 300")
- Dark gradient background, subtle blue accent border on word box
- Auto-rotates on timer, no tap/swipe interaction

### Settings Menu

Opened via long press (2 seconds). Simple list UI:

- **Words per day**: 5 / 10 / 15 / 20 / 25 (default: 15)
- **Display duration**: 30s / 1min / 2min / 5min (default: 1min)
- **Brightness**: Low / Medium / High (default: Medium)

Settings persisted to SPIFFS across reboots.

## Image System

- ~300 pre-rendered 120x120 RGB565 bitmaps
- RLE compressed at build time (~4-6KB each)
- Stored in SPIFFS data/ directory
- Python build script (tools/convert_emoji.py) converts emoji PNGs to .bin format
- Decompressed into display buffer on card change

## Word Data

- ~300 most common Spanish nouns, verbs, and adjectives with clear emoji representations
- Ranked by word frequency (most common words first)
- Abstract words (but, with, because, very) excluded - every word has a meaningful image
- Stored as a C header array (word_data.h): Spanish word, English translation, image filename, category

## Card Rotation Logic

1. On boot, load settings and progress state from SPIFFS
2. Select the day's batch of words (based on words_per_day setting) in frequency order
3. Randomly shuffle the day's batch
4. Display each card for the configured duration, cycling through the batch
5. After all words shown, loop back to start of today's batch
6. At midnight rollover, advance to next batch of unseen words
7. Track progress index in SPIFFS - once all ~300 words covered, restart from word 1

## Optimal Learning Notes

Research on vocabulary acquisition suggests:

- 10-20 new words per day is the sweet spot for adult learners
- Default of 15 words/day balances exposure with retention
- At 15 words/day, all 300 words covered in 20 days
- Passive exposure (Osmosis style) works best as a supplement - repeated visual association builds familiarity
- 1-minute display duration allows enough time to read and absorb without rushing

## Project Structure

```
Osmosis/
  platformio.ini
  src/
    main.cpp              - Boot, main loop, state management
    display_manager.cpp/h - TFT init, drawing primitives, image rendering
    touch_handler.cpp/h   - Touch detection, long press detection
    card_manager.cpp/h    - Word selection, rotation, progress tracking
    settings_manager.cpp/h - Load/save settings to SPIFFS
    splash_screen.cpp/h   - Splash screen rendering
    ui_settings.cpp/h     - Settings menu UI
    word_data.h           - Word list array (generated or hand-curated)
  data/                   - SPIFFS partition (emoji bitmaps)
    img_casa.bin
    img_perro.bin
    ...
  tools/
    convert_emoji.py      - Build-time emoji-to-RGB565 converter
  docs/
    plans/
```

## Dependencies

- bodmer/TFT_eSPI (same display config as Tide Clock)
- PaulStoffregen/XPT2046_Touchscreen
- SPIFFS (built-in)

## Out of Scope for v1.0

- WiFi connectivity
- Downloadable language packs
- Spaced repetition algorithm (SRS)
- Quiz/test mode
- Audio pronunciation
- OTA firmware updates
