# Osmosis v3 Expansion Design

Date: 2026-02-21

## Overview

Four workstreams to bring Osmosis from a working prototype to a polished product:

1. Expand from ~280 words to 300 curated emoji vocabulary across 5 languages x 4 tiers (20 packs)
2. Fix the download progress bar (currently stuck at 0%)
3. Build a product website at vcodeworks.dev/osmosis
4. Improve the WiFi captive portal with network scanning

## 1. Language Pack Expansion

### Vocabulary: 300 Curated Emoji

300 emoji selected from the full Unicode set, filtered to:
- One emoji per concept (no skin tone/gender variants)
- English word maps to a top-1000 frequency word
- Concrete nouns, basic emotions, core actions

### Tier Structure (75 words each)

| Tier | Focus | Examples |
|------|-------|---------|
| **Beginner** | Core nouns — body, family, food, animals, home, nature | water, house, dog, bread, sun, hand |
| **Intermediate** | Daily life — clothing, transport, places, kitchen, emotions | shirt, car, school, fork, angry |
| **Advanced** | Abstract — actions, communication, sports, time, events | running, speak, basketball, night, birthday |
| **Expert** | Extended — more animals, objects, nature, colors, miscellaneous | giraffe, sword, desert, red, magnet |

### Languages

All 5 currently supported: Spanish, French, Portuguese, Chinese, Dutch.

Each language gets all 4 tiers = 20 total packs.

### Translation Approach

- Use web search for accurate translations + phonetic pronunciation guides
- Spanish: verify against existing 284-word packs where overlap exists
- Chinese: "translation" = pinyin, "phonetic" = pronunciation guide
- All phonetics in ALL-CAPS format matching existing convention (e.g., "AH-gwah")

### Emoji .bin Generation

- 264 emoji already on CDN
- ~36 new codepoints need rendering via `render_apple_emoji.py` + `convert_emoji.py`
- Push all new .bin files to CDN

### Build Pipeline Changes

- Update `build_all_packs.sh` for 20 packs (5 languages x 4 tiers)
- Update `build_pack.py` LANG_DISPLAY and TIER_DISPLAY dicts to include "expert"
- Update `catalog.json` with all 20 pack entries
- Font files: one per language, shared across tiers (already the case)

## 2. Progress Bar Fix

### Problem

Download is synchronous in `startDownload()`. The progress screen is drawn once before the blocking call starts, but the main loop never runs during download, so the screen never updates. Shows 0% then jumps to "Pack Installed!".

### Solution

Add a screen redraw callback inside the emoji download loop in `pack_manager.cpp`. After each emoji downloads, redraw all 32 strips of the progress screen. The progress values (`_progress`, `_statusBuf`) are already being updated correctly — they just need to be rendered.

### Implementation

Add a function pointer or direct call to redraw progress:

```
// In the emoji download loop (pack_manager.cpp line ~236):
for (uint16_t i = 0; i < _emojiCount; i++) {
    // ... download emoji ...
    _emojiDone++;
    _progress = 25 + (_emojiDone * 65 / _emojiTotal);
    snprintf(_statusBuf, ...);

    // NEW: Redraw progress screen
    redrawProgressScreen();

    yield();
}
```

The redraw function iterates all 32 strips, drawing the progress bar and status text. This gives smooth visual feedback from 0% through 100%.

Also update progress during manifest (5%) and font (15-25%) download phases by adding redraw calls there too.

## 3. Product Website

### Location

New page at `/osmosis/index.html` in the vcwebsite repo. Main VCodeworks landing page stays untouched.

### Content (single page, mobile-friendly)

1. **Hero**: "OSMOSIS" title + tagline "Learn a language, one word at a time"
2. **Product description**: Brief explanation of how the device works
3. **Sample flashcards**: Screenshots/mockups showing emoji + word box
4. **Language packs section**:
   - Tier descriptions (Beginner/Intermediate/Advanced/Expert with category descriptions)
   - Grid showing languages x tiers with word counts
5. **Contact**: "Want another language? Email kevin@vcodeworks.dev"
6. **Footer**: VCodeworks branding

### Design Style

Dark theme matching the device:
- Black background (#0a0a1a)
- Purple-blue accents (#4e7fff, matching CLR_ACCENT)
- White/light gray text
- Rounded cards for content sections
- Mobile-first responsive layout

## 4. Prettier Captive Portal

### Current State

Minimal HTML form with manual SSID text entry + password field. No network scanning. Device TFT shows basic "Join Osmosis-Setup WiFi" text.

### Improvements

#### Portal Web Page
1. ESP32 scans WiFi networks before serving the portal page
2. Network list embedded as JSON in the HTML
3. Portal shows clickable network cards with name + signal strength indicator
4. Selecting a network fills the SSID field; user just enters password
5. Styled to match Osmosis branding (dark theme, purple-blue accents)
6. Shows `www.vcodeworks.dev` link for product info

#### Device TFT Screen (during portal mode)
- Clear instructions: "Join 'Osmosis-Setup' WiFi"
- "Then open your browser"
- Show "www.vcodeworks.dev" website URL

### Technical Notes

- `WiFi.scanNetworks()` runs in STA+AP mode to scan while AP is active
- Network list sorted by signal strength, duplicates removed
- Portal HTML stays in PROGMEM (ESP32 flash) to avoid SPIFFS dependency
- Scan happens once when portal starts; "Refresh" button to rescan
