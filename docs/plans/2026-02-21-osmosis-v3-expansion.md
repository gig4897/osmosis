# Osmosis v3 Expansion Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Expand Osmosis from prototype to polished product with 300-word vocabulary across 5 languages x 4 tiers, working progress bar, product website, and improved WiFi portal.

**Architecture:** Four independent workstreams: (1) vocabulary CSV generation + pack building + CDN deployment, (2) firmware progress bar fix via redraw callback during synchronous download, (3) static HTML product page at /osmosis/ on Cloudflare Pages, (4) enhanced captive portal with WiFi scanning in PROGMEM HTML.

**Tech Stack:** ESP32 Arduino (C++), Python 3 (pack tools), HTML/CSS/JS (website + portal), PlatformIO, Cloudflare Pages CDN.

---

## Task 1: Build Master 300-Word Emoji Reference

**Files:**
- Create: `tools/vocab/master_emoji_300.csv`

**Step 1: Create the master reference CSV**

This file defines all 300 emoji codepoints and their English words. It is the source of truth — all language CSVs reference these same 300 entries.

Format: `codepoint,english,category,tier`

The 300 words divided into 4 tiers of 75:

**Beginner (75):** baby, boy, girl, man, woman, person, child, family, hand, arm, leg, foot, ear, nose, eye, mouth, tongue, tooth, heart, water, fire, star, sun, moon, earth, rain, snow, dog, cat, mouse, rabbit, bear, pig, cow, horse, sheep, monkey, elephant, lion, bird, chicken, fish, snake, frog, bee, ant, tree, flower, plant, leaf, rose, apple, banana, orange, lemon, grape, strawberry, tomato, potato, carrot, corn, bread, cheese, egg, milk, rice, tea, coffee, house, door, bed, light, book, key, ball

**Intermediate (75):** brain, kiss, blood, cloud, sea, wind, rainbow, rock, ice, tiger, wolf, fox, deer, duck, eagle, owl, turtle, dolphin, whale, butterfly, mushroom, seed, wood, pear, peach, cherry, watermelon, onion, meat, pizza, soup, cake, chocolate, candy, honey, salt, beer, wine, bottle, glass, juice, building, school, hospital, store, church, car, bus, train, airplane, boat, bicycle, truck, window, chair, television, computer, phone, camera, candle, letter, money, paper, picture, glasses, shirt, pants, dress, shoe, hat, coat, umbrella, bag, ring, tie

**Advanced (75):** face, happy, sad, angry, afraid, tired, surprised, sick, laugh, cry, love, speak, think, running, swimming, walking, dancing, writing, fishing, skiing, surfing, morning, evening, night, birthday, party, gift, music, song, piano, guitar, drum, movie, art, game, bell, hotel, city, mountain, beach, road, bank, castle, island, park, taxi, helicopter, rocket, ambulance, motorcycle, ship, radio, newspaper, note, pen, pencil, scissors, calendar, lock, hammer, knife, fork, spoon, plate, map, box, mail, bath, soap, coin, clock, watch, flag, sword, gun

**Expert (75):** goat, penguin, shark, giraffe, zebra, kangaroo, snail, octopus, crab, crocodile, gorilla, unicorn, spider, police, prince, princess, basketball, tennis, baseball, boxing, hamburger, ice cream, cookie, cooking, steak, garlic, coconut, pepper, straw, bowl, mirror, crown, cart, lightning, scarf, gloves, bone, balloon, fireworks, battery, broom, dove, microphone, alarm, forest, desert, bridge, blossom, sunflower, autumn, cactus, chain, axe, basket, wheel, feather, needle, thread, magnet, sparkler, circus, arrow, hour, red, blue, green, yellow, purple, black, white, wall, wood (dup-check: already in intermediate, swap for another), telephone, picture (dup-check)

Note: During implementation, deduplicate any words that appear in multiple tiers and swap replacements from the broader emoji set.

**Step 2: Validate the master list**

Run: `python3 -c "import csv; r=csv.DictReader(open('tools/vocab/master_emoji_300.csv')); rows=list(r); print(f'{len(rows)} words, {len(set(r[\"codepoint\"] for r in rows))} unique codepoints, {len(set(r[\"english\"] for r in rows))} unique english words')"`

Expected: 300 words, 300 unique codepoints, 300 unique english words

---

## Task 2: Generate Language Vocab CSVs (5 languages x 4 tiers)

**Files:**
- Create: `tools/generate_translations.py`
- Create: 20 CSV files in `tools/vocab/` (e.g., `spanish_beginner.csv`, `spanish_intermediate.csv`, `spanish_advanced.csv`, `spanish_expert.csv`, `french_beginner.csv`, etc.)

**Step 1: Create the translation generator script**

This script takes the master_emoji_300.csv and a language, then uses a reference data structure (hardcoded translations + phonetics for each language) to produce the 4 tier CSVs.

The script should:
1. Read `master_emoji_300.csv`
2. Filter by tier
3. Look up translation + phonetic from a language-specific dictionary
4. Output CSV in the format: `emoji,english,translation,phonetic,category`

**Step 2: Build Spanish translations (300 words)**

Build on existing Spanish data (we have 284 words already). Fill remaining ~16 gaps. Verify all translations and phonetics are accurate via web search.

Output: `tools/vocab/spanish_beginner.csv` (75 words), `spanish_intermediate.csv` (75), `spanish_advanced.csv` (75), `spanish_expert.csv` (75)

**Step 3: Build French translations (300 words)**

Create all 300 French translations + phonetic guides.

Output: `tools/vocab/french_beginner.csv` through `french_expert.csv`

**Step 4: Build Portuguese translations (300 words)**

Output: `tools/vocab/portuguese_beginner.csv` through `portuguese_expert.csv`

**Step 5: Build Chinese translations (300 words)**

Translation = pinyin, phonetic = pronunciation guide.

Output: `tools/vocab/chinese_beginner.csv` through `chinese_expert.csv`

**Step 6: Build Dutch translations (300 words)**

Output: `tools/vocab/dutch_beginner.csv` through `dutch_expert.csv`

**Step 7: Validate all CSVs**

Run: `for f in tools/vocab/*.csv; do echo "$(basename $f): $(tail -n +2 "$f" | wc -l) words"; done`

Expected: Each of the 20 CSVs has exactly 75 words. Old CSVs (spanish_beginner.csv etc.) are overwritten with the new 75-word versions.

---

## Task 3: Render New Emoji .bin Files

**Files:**
- Output: new .bin files in `tools/emoji_bin/`

**Step 1: Identify which emoji codepoints need new .bin files**

Cross-reference the 300 master codepoints against existing .bin files on CDN (264 already exist).

Run:
```python
python3 << 'EOF'
import csv, urllib.request, concurrent.futures

with open('tools/vocab/master_emoji_300.csv') as f:
    codepoints = [row['codepoint'] for row in csv.DictReader(f)]

def check(cp):
    try:
        req = urllib.request.Request(f"https://www.vcodeworks.dev/api/osmosis/packs/emoji/{cp}.bin", method='HEAD')
        urllib.request.urlopen(req, timeout=10)
        return (cp, True)
    except:
        return (cp, False)

with concurrent.futures.ThreadPoolExecutor(20) as ex:
    results = list(ex.map(lambda cp: check(cp), codepoints))

missing = [cp for cp, ok in results if not ok]
print(f"{len(missing)} new emoji to render: {missing}")
EOF
```

**Step 2: Render missing emoji PNGs**

Run: `python3 tools/render_apple_emoji.py tools/emoji_apple_png/ --csv tools/vocab/`

This renders Apple Color Emoji for any codepoints found in vocab CSVs that don't have PNGs yet.

**Step 3: Convert PNGs to ORLE .bin**

Run: `python3 tools/convert_emoji.py tools/emoji_apple_png/ tools/emoji_bin/`

**Step 4: Verify .bin files exist for all 300 codepoints**

Run: `python3 -c "import csv; cps=[r['codepoint'] for r in csv.DictReader(open('tools/vocab/master_emoji_300.csv'))]; import os; missing=[cp for cp in cps if not os.path.exists(f'tools/emoji_bin/{cp}.bin')]; print(f'{len(missing)} missing' if missing else 'All 300 present')"`

Expected: "All 300 present"

---

## Task 4: Build All Packs and Deploy to CDN

**Files:**
- Modify: `tools/build_pack.py` (add "expert" to TIER_DISPLAY)
- Modify: `tools/build_all_packs.sh` (add all 20 packs)
- Modify: `/Users/kevintomlinson/Coding/vcwebsite/api/osmosis/catalog.json`
- Copy: manifests, fonts, emoji to vcwebsite repo

**Step 1: Update build_pack.py**

Add to TIER_DISPLAY dict:
```python
TIER_DISPLAY = {
    "beginner": "Beginner", "intermediate": "Intermediate",
    "advanced": "Advanced", "expert": "Expert"
}
```

**Step 2: Update build_all_packs.sh**

Rewrite to build all 20 packs:
```bash
for lang in spanish french portuguese chinese dutch; do
    for tier in beginner intermediate advanced expert; do
        python3 tools/build_pack.py \
            --csv tools/vocab/${lang}_${tier}.csv \
            --output packs/${lang}/${tier}/ \
            --language $lang --tier $tier
    done
    python3 tools/generate_vlw_font.py --language "$lang" --size 26 --output "packs/$lang/font.vlw"
done
```

**Step 3: Run the build**

Run: `cd /Users/kevintomlinson/Coding/Osmosis && ./tools/build_all_packs.sh`

Expected: 20 manifest.json files + 5 font.vlw files generated.

**Step 4: Update catalog.json**

Edit `/Users/kevintomlinson/Coding/vcwebsite/api/osmosis/catalog.json` to list all 5 languages with 4 tiers each, with correct word counts (75 per tier).

**Step 5: Copy pack files to CDN repo**

```bash
cd /Users/kevintomlinson/Coding/vcwebsite

# Copy manifests and fonts
for lang in spanish french portuguese chinese dutch; do
    for tier in beginner intermediate advanced expert; do
        mkdir -p api/osmosis/packs/$lang/$tier
        cp /Users/kevintomlinson/Coding/Osmosis/packs/$lang/$tier/manifest.json api/osmosis/packs/$lang/$tier/
    done
    # Font is per-language, copy to each tier directory
    for tier in beginner intermediate advanced expert; do
        cp /Users/kevintomlinson/Coding/Osmosis/packs/$lang/font.vlw api/osmosis/packs/$lang/$tier/
    done
done

# Copy any new emoji .bin files
cp /Users/kevintomlinson/Coding/Osmosis/tools/emoji_bin/*.bin api/osmosis/packs/emoji/
```

**Step 6: Push to CDN**

```bash
cd /Users/kevintomlinson/Coding/vcwebsite
git add .
git commit -m "Expand to 300 words x 5 languages x 4 tiers (20 packs)"
git push
```

Cloudflare Pages deploys within ~1 minute.

**Step 7: Verify deployment**

Spot-check a few new pack URLs:
```bash
curl -s -o /dev/null -w "%{http_code}" https://www.vcodeworks.dev/api/osmosis/packs/french/expert/manifest.json
curl -s -o /dev/null -w "%{http_code}" https://www.vcodeworks.dev/api/osmosis/packs/dutch/advanced/manifest.json
curl -s -o /dev/null -w "%{http_code}" https://www.vcodeworks.dev/api/osmosis/catalog.json
```

Expected: All return 200.

---

## Task 5: Fix Download Progress Bar

**Files:**
- Modify: `src/pack_manager.cpp`
- Modify: `src/pack_manager.h`
- Modify: `src/ui_settings.cpp`

**Step 1: Add a progress redraw callback to pack_manager**

In `src/pack_manager.h`, add a callback function pointer:

```cpp
namespace packMgr {
    // ... existing declarations ...

    // Progress callback — called during download to allow UI redraw
    typedef void (*ProgressCallback)();
    void setProgressCallback(ProgressCallback cb);
}
```

In `src/pack_manager.cpp`, add the static variable and setter:

```cpp
static ProgressCallback _progressCb = nullptr;

void setProgressCallback(ProgressCallback cb) { _progressCb = cb; }
```

**Step 2: Call the callback during download phases**

In `startDownload()`, add callback calls after each progress update:

After manifest download (~line 177):
```cpp
_progress = 15;
if (_progressCb) _progressCb();
```

After font download (~line 189):
```cpp
_progress = 25;
if (_progressCb) _progressCb();
```

Inside the emoji download loop (~line 256):
```cpp
_emojiDone++;
_progress = 25 + (_emojiDone * 65 / _emojiTotal);
if (_progressCb) _progressCb();
yield();
```

After cleanup (~line 263):
```cpp
_progress = 92;
if (_progressCb) _progressCb();
```

**Step 3: Wire up the callback in ui_settings.cpp**

Before calling `startDownload()`, set the callback to redraw the progress screen:

```cpp
// Set progress callback to redraw screen during download
packMgr::setProgressCallback([]() {
    TFT_eSprite& spr = display.getStrip();
    for (int strip = 0; strip < NUM_STRIPS; strip++) {
        int sy = strip * STRIP_H;
        spr.fillSprite(CLR_BG_DARK);
        settingsUI.drawDownloadProgress(spr, sy);
        display.pushStrip(sy);
    }
});

bool ok = packMgr::startDownload(_selectedLang, i);

// Clear callback after download
packMgr::setProgressCallback(nullptr);
```

Note: `drawDownloadProgress` is currently private. Make it public in `ui_settings.h`, or add a dedicated redraw method.

**Step 4: Make drawDownloadProgress accessible**

In `src/ui_settings.h`, move `drawDownloadProgress` to public:

```cpp
public:
    // ... existing public methods ...
    void drawDownloadProgress(TFT_eSprite& spr, int stripY);
```

**Step 5: Build and flash**

Run: `cd /Users/kevintomlinson/Coding/Osmosis && pio run -t upload`

Expected: Clean build, successful flash. Download a pack and watch progress bar animate from 0% to 100%.

---

## Task 6: Build Product Website

**Files:**
- Create: `/Users/kevintomlinson/Coding/vcwebsite/osmosis/index.html`

**Step 1: Create the Osmosis product page**

Single-page, mobile-first, dark theme. Sections:

1. **Hero**: OSMOSIS title, tagline "Learn a language, one word at a time", brief device description
2. **How It Works**: 3-step flow with emoji icons (Connect WiFi, Browse Languages, Learn)
3. **Language Packs**: Tier descriptions + language availability grid
   - Beginner: "Core nouns — body, family, food, animals, home, nature"
   - Intermediate: "Daily life — clothing, transport, places, kitchen, emotions"
   - Advanced: "Abstract — actions, communication, sports, time, events"
   - Expert: "Extended — more animals, objects, nature, colors, miscellaneous"
   - 5 languages x 4 tiers, 75 words each, 300 total per language
4. **Sample Cards**: CSS mockups of flashcard UI showing emoji + word box
5. **Contact**: "Want another language? Email kevin@vcodeworks.dev"
6. **Footer**: VCodeworks branding

Design tokens:
- Background: `#0a0a1a`
- Accent: `#4e7fff` (matches device CLR_ACCENT in RGB)
- Text: `#ffffff` / `#b0b0b0`
- Cards: `#1a1a2e` with subtle border
- Font: system sans-serif stack
- Max width: 800px centered

All CSS inline (single file, no build step). No JavaScript required.

**Step 2: Push to CDN**

```bash
cd /Users/kevintomlinson/Coding/vcwebsite
git add osmosis/
git commit -m "Add Osmosis product page"
git push
```

Verify: `curl -s -o /dev/null -w "%{http_code}" https://www.vcodeworks.dev/osmosis/`

Expected: 200

---

## Task 7: Improve Captive Portal

**Files:**
- Modify: `src/wifi_manager.cpp`
- Modify: `src/wifi_manager.h`
- Modify: `src/ui_settings.cpp` (portal mode display on TFT)

**Step 1: Add WiFi scan function**

In `wifi_manager.h`:
```cpp
namespace wifiMgr {
    // ... existing ...
    int scanNetworks();                    // Scan and return count
    const char* scannedSSID(int index);    // Get SSID by index
    int32_t scannedRSSI(int index);        // Get RSSI by index
}
```

In `wifi_manager.cpp`:
```cpp
static int _scanCount = 0;

int scanNetworks() {
    WiFi.mode(WIFI_AP_STA);  // AP + STA to scan while AP is active
    _scanCount = WiFi.scanNetworks(false, false, false, 300);
    Serial.printf("[wifi] Scan found %d networks\n", _scanCount);
    return _scanCount;
}

const char* scannedSSID(int i) { return WiFi.SSID(i).c_str(); }
int32_t scannedRSSI(int i) { return WiFi.RSSI(i); }
```

**Step 2: Rebuild PORTAL_HTML with network list**

Replace the static PORTAL_HTML with a function that generates HTML dynamically, injecting scanned networks as clickable buttons sorted by signal strength.

The new portal page:
- Dark theme matching Osmosis branding (#0a0a1a bg, #4e7fff accent)
- "OSMOSIS" header + "WiFi Setup" subtitle
- Scanned network list as clickable cards showing SSID + signal bars
- Clicking a network selects it and shows the password field
- Password input + Connect button
- "Refresh" link to rescan
- Footer: "Visit www.vcodeworks.dev for more info"

Since the HTML must include the dynamic network list, generate it as a String instead of PROGMEM:

```cpp
static String buildPortalHtml() {
    String html = F("<!DOCTYPE html><html>...");
    // Inject network buttons from scan results
    for (int i = 0; i < _scanCount && i < 15; i++) {
        html += "<div class='net' onclick=\"sel('" + WiFi.SSID(i) + "')\">";
        html += WiFi.SSID(i);
        html += " (" + String(WiFi.RSSI(i)) + " dBm)";
        html += "</div>";
    }
    html += F("...</html>");
    return html;
}
```

**Step 3: Call scan before starting portal**

In `startCaptivePortal()`, add scan before starting the web server:

```cpp
void startCaptivePortal() {
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP("Osmosis-Setup");
    delay(100);
    scanNetworks();  // Scan before serving
    // ... rest of portal setup ...
}
```

**Step 4: Add rescan endpoint**

```cpp
_server->on("/scan", HTTP_GET, []() {
    scanNetworks();
    _server->sendHeader("Location", "/");
    _server->send(302, "text/plain", "");
});
```

**Step 5: Update TFT display during portal mode**

In `ui_settings.cpp` `drawLanguageBrowser()`, when `CaptivePortalActive`, show clearer instructions:

```
Line 1: "Join 'Osmosis-Setup' WiFi"
Line 2: "Then open your browser"
Line 3: ""
Line 4: "www.vcodeworks.dev"
```

**Step 6: Build and flash**

Run: `cd /Users/kevintomlinson/Coding/Osmosis && pio run -t upload`

Expected: Clean build. When WiFi not configured and user opens language browser, captive portal starts, phone connects to Osmosis-Setup AP, browser shows network list with signal strength.

---

## Task 8: Final Verification and Commit

**Step 1: Flash firmware and test full flow**

1. Erase SPIFFS: `pio run -t erase` then `pio run -t upload`
2. Device boots to NoPack state
3. Long press → Settings → Browse Languages → Captive portal starts
4. Connect phone to Osmosis-Setup, open browser, see network list
5. Select network, enter password, connect
6. Browse languages → see 5 languages with 4 tiers each
7. Download Spanish Beginner → progress bar animates smoothly
8. Device reboots, shows flashcards with 75 words

**Step 2: Test serial monitor during download**

```bash
python3 -c "
import serial, sys
ser = serial.Serial('/dev/cu.usbserial-110', 115200, timeout=1)
ser.dtr = False; ser.rts = False
while True:
    line = ser.readline()
    if line: sys.stdout.write(line.decode('utf-8', errors='replace')); sys.stdout.flush()
"
```

Verify emoji download progress messages appear in sequence.

**Step 3: Verify website**

Open: `https://www.vcodeworks.dev/osmosis/`

Verify: Page loads, shows all 5 languages, tier descriptions match design, contact email is correct, mobile responsive.

**Step 4: Update CLAUDE.md**

Update the "Current Language Packs" table and any other sections that reference the old pack structure.
